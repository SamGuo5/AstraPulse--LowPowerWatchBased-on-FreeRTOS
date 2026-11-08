#include "storage_service.hpp"

#include "FreeRTOS.h"
#include "bsp.h"
#include "queue.h"
#include "task.h"
#include "system_monitor_service.hpp"
#include <array>
#include <cstring>

namespace {
constexpr uint32_t record_magic=0x57534a31u;  // "WSJ1"
constexpr uint32_t flash_size_required=0x200000u;
constexpr uint32_t page_size=0x2000u;
constexpr uint32_t page_address[2]={0x081fc000u,0x081fe000u};
constexpr uint32_t bank_page[2]={126u,127u};
constexpr uint16_t records_per_page=page_size/16u;
constexpr uint8_t key_capacity=8u;
constexpr uint8_t queue_length=12u;

struct alignas(16) JournalRecord {
  uint32_t magic;
  uint32_t meta;
  uint32_t value;
  uint32_t checksum;
};
static_assert(sizeof(JournalRecord)==16u,"Flash journal record must be one quad-word");

struct WriteCommand {StorageKey key;uint32_t value;};
struct PageScan {uint32_t highest_sequence;uint16_t next_slot;bool has_valid;bool blank;};

std::array<uint32_t,key_capacity> values{};
std::array<uint32_t,key_capacity> sequences{};
std::array<bool,key_capacity> present{};
QueueHandle_t command_queue;
StaticQueue_t command_queue_control;
uint8_t command_queue_storage[queue_length*sizeof(WriteCommand)];
StaticTask_t task_control;
StackType_t task_stack[512];
StorageStatus status{};
uint32_t next_sequence=1u;
uint8_t active_page{};
uint16_t active_slot{};

uint8_t key_index(StorageKey key){return static_cast<uint8_t>(key);}
bool valid_key(uint8_t key){return key>0u&&key<key_capacity;}

uint32_t crc32(const void *data,size_t length){
  const auto *bytes=static_cast<const uint8_t *>(data);uint32_t crc=0xffffffffu;
  for(size_t index=0;index<length;++index){
    crc^=bytes[index];
    for(uint8_t bit=0;bit<8u;++bit)crc=(crc>>1u)^(0xedb88320u&static_cast<uint32_t>(-(int32_t)(crc&1u)));
  }
  return ~crc;
}

bool blank(const JournalRecord &record){
  return record.magic==0xffffffffu&&record.meta==0xffffffffu&&
         record.value==0xffffffffu&&record.checksum==0xffffffffu;
}

uint32_t sequence_of(const JournalRecord &record){return record.meta>>8u;}
uint8_t record_key(const JournalRecord &record){return static_cast<uint8_t>(record.meta&0xffu);}
bool newer(uint32_t candidate,uint32_t reference){
  const uint32_t delta=(candidate-reference)&0x00ffffffu;
  return delta!=0u&&delta<0x00800000u;
}

bool valid_record(const JournalRecord &record){
  return record.magic==record_magic&&valid_key(record_key(record))&&
         record.checksum==crc32(&record,12u);
}

JournalRecord read_record(uint8_t page,uint16_t slot){
  JournalRecord record{};
  const auto *source=reinterpret_cast<const void *>(page_address[page]+slot*sizeof(JournalRecord));
  std::memcpy(&record,source,sizeof(record));return record;
}

PageScan scan_page(uint8_t page){
  PageScan scan{0u,0u,false,true};
  for(uint16_t slot=0;slot<records_per_page;++slot){
    const JournalRecord record=read_record(page,slot);
    if(blank(record))continue;
    scan.blank=false;scan.next_slot=static_cast<uint16_t>(slot+1u);
    if(!valid_record(record))continue;
    const uint8_t key=record_key(record);const uint32_t sequence=sequence_of(record);
    if(!present[key]||newer(sequence,sequences[key])){
      present[key]=true;sequences[key]=sequence;values[key]=record.value;
    }
    if(!scan.has_valid||newer(sequence,scan.highest_sequence)){
      scan.has_valid=true;scan.highest_sequence=sequence;
    }
  }
  return scan;
}

void publish_error(){
  taskENTER_CRITICAL();if(status.error_count<0xffffu)++status.error_count;taskEXIT_CRITICAL();
}

bool erase_page(uint8_t page){
  FLASH_EraseInitTypeDef erase{};erase.TypeErase=FLASH_TYPEERASE_PAGES;
  erase.Banks=FLASH_BANK_2;erase.Page=bank_page[page];erase.NbPages=1u;
  uint32_t page_error=0xffffffffu;
  if(HAL_FLASH_Unlock()!=HAL_OK)return false;
  const HAL_StatusTypeDef result=HAL_FLASHEx_Erase(&erase,&page_error);
  (void)HAL_FLASH_Lock();
  if(result!=HAL_OK||page_error!=0xffffffffu)return false;
  return blank(read_record(page,0u));
}

bool page_is_blank(uint8_t page){
  for(uint16_t slot=0;slot<records_per_page;++slot)if(!blank(read_record(page,slot)))return false;
  return true;
}

JournalRecord make_record(StorageKey key,uint32_t value){
  JournalRecord record{record_magic,
    ((next_sequence&0x00ffffffu)<<8u)|static_cast<uint8_t>(key),value,0u};
  record.checksum=crc32(&record,12u);
  next_sequence=(next_sequence+1u)&0x00ffffffu;if(next_sequence==0u)next_sequence=1u;
  return record;
}

bool program_record(uint8_t page,uint16_t slot,StorageKey key,uint32_t value){
  if(slot>=records_per_page)return false;
  const uint32_t address=page_address[page]+slot*sizeof(JournalRecord);
  if(!blank(read_record(page,slot)))return false;
  alignas(16) JournalRecord record=make_record(key,value);
  if(HAL_FLASH_Unlock()!=HAL_OK)return false;
  const HAL_StatusTypeDef result=HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,address,
                                                   reinterpret_cast<uint32_t>(&record));
  (void)HAL_FLASH_Lock();
  if(result!=HAL_OK)return false;
  const JournalRecord verify=read_record(page,slot);
  return std::memcmp(&verify,&record,sizeof(record))==0;
}

bool compact(){
  const uint8_t target=static_cast<uint8_t>(active_page^1u);
  if(!page_is_blank(target)){
    uint8_t count=0;
    for(uint8_t key=1u;key<key_capacity;++key)if(present[key])++count;
    if(static_cast<uint32_t>(active_slot)+count>records_per_page)return false;
    /* Complete the current page first.  If power fails while the stale target
       is erased or repopulated, this checkpoint still contains every key. */
    for(uint8_t key=1u;key<key_capacity;++key){
      if(!present[key])continue;
      if(!program_record(active_page,active_slot++,static_cast<StorageKey>(key),values[key]))return false;
    }
  }
  if(!erase_page(target))return false;
  uint16_t slot=0;
  for(uint8_t key=1u;key<key_capacity;++key){
    if(!present[key])continue;
    if(!program_record(target,slot++,static_cast<StorageKey>(key),values[key]))return false;
  }
  const uint8_t previous=active_page;active_page=target;active_slot=slot;
  taskENTER_CRITICAL();status.active_page=active_page;taskEXIT_CRITICAL();
  if(!erase_page(previous))publish_error();
  return true;
}

bool persist(StorageKey key,uint32_t value){
  if(active_slot>=records_per_page-key_capacity&&!compact())return false;
  if(!program_record(active_page,active_slot,key,value))return false;
  ++active_slot;
  taskENTER_CRITICAL();++status.successful_writes;taskEXIT_CRITICAL();return true;
}

void storage_task(void *){
  for(;;){
    WriteCommand command{};
    if(xQueueReceive(command_queue,&command,portMAX_DELAY)!=pdTRUE)continue;
    if(!persist(command.key,command.value))publish_error();
  }
}
}

void storage_service_start(){
  command_queue=xQueueCreateStatic(queue_length,sizeof(WriteCommand),command_queue_storage,&command_queue_control);
  configASSERT(command_queue);
  if(FLASH_SIZE<flash_size_required){
    status.ready=false;status.error_count=1u;
  }else{
    const PageScan first=scan_page(0u);const PageScan second=scan_page(1u);
    if(second.has_valid&&(!first.has_valid||newer(second.highest_sequence,first.highest_sequence)))active_page=1u;
    else active_page=0u;
    const PageScan &active=active_page==0u?first:second;
    active_slot=active.next_slot;
    uint32_t highest=0u;bool found=false;
    for(uint8_t key=1u;key<key_capacity;++key)if(present[key]&&(!found||newer(sequences[key],highest))){highest=sequences[key];found=true;}
    next_sequence=found?((highest+1u)&0x00ffffffu):1u;if(next_sequence==0u)next_sequence=1u;
    status.ready=true;status.active_page=active_page;
  }
  TaskHandle_t handle=xTaskCreateStatic(storage_task,"storage",512,nullptr,1,task_stack,&task_control);
  configASSERT(handle);system_monitor_register(handle);
}

bool storage_service_read(StorageKey key,uint32_t &value){
  const uint8_t index=key_index(key);if(!valid_key(index))return false;
  taskENTER_CRITICAL();const bool exists=present[index];const uint32_t copy=values[index];taskEXIT_CRITICAL();
  if(exists)value=copy;
  return exists;
}

bool storage_service_write(StorageKey key,uint32_t value){
  const uint8_t index=key_index(key);if(!valid_key(index)||!status.ready)return false;
  taskENTER_CRITICAL();
  const bool unchanged=present[index]&&values[index]==value;
  taskEXIT_CRITICAL();
  if(unchanged)return true;
  const WriteCommand command{key,value};
  if(xQueueSend(command_queue,&command,0)!=pdTRUE){publish_error();return false;}
  taskENTER_CRITICAL();present[index]=true;values[index]=value;taskEXIT_CRITICAL();return true;
}

StorageStatus storage_service_status(){
  taskENTER_CRITICAL();const StorageStatus copy=status;taskEXIT_CRITICAL();return copy;
}
