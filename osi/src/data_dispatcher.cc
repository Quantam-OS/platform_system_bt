/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "bt_osi_data_dispatcher"

#include "osi/include/data_dispatcher.h"

#include <assert.h>
#include <unordered_map>

#include "osi/include/allocator.h"
#include "osi/include/osi.h"
#include "osi/include/log.h"

#define DEFAULT_TABLE_BUCKETS 10

typedef std::unordered_map<data_dispatcher_type_t, fixed_queue_t*> DispatchTableMap;

struct data_dispatcher_t {
  char *name;
  DispatchTableMap *dispatch_table;
  fixed_queue_t *default_queue; // We don't own this queue
};

data_dispatcher_t *data_dispatcher_new(const char *name) {
  assert(name != NULL);

  data_dispatcher_t *ret = (data_dispatcher_t*)osi_calloc(sizeof(data_dispatcher_t));

  ret->dispatch_table = new DispatchTableMap();

  ret->name = osi_strdup(name);
  if (!ret->name) {
    LOG_ERROR(LOG_TAG, "%s unable to duplicate provided name.", __func__);
    goto error;
  }

  return ret;

error:;
  data_dispatcher_free(ret);
  return NULL;
}

void data_dispatcher_free(data_dispatcher_t *dispatcher) {
  if (!dispatcher)
    return;

  delete dispatcher->dispatch_table;
  osi_free(dispatcher->name);
  osi_free(dispatcher);
}

void data_dispatcher_register(data_dispatcher_t *dispatcher, data_dispatcher_type_t type, fixed_queue_t *queue) {
  assert(dispatcher != NULL);

  if (queue)
    (*dispatcher->dispatch_table)[type] = queue;
  else
    dispatcher->dispatch_table->erase(type);

}

void data_dispatcher_register_default(data_dispatcher_t *dispatcher, fixed_queue_t *queue) {
  assert(dispatcher != NULL);

  dispatcher->default_queue = queue;
}

bool data_dispatcher_dispatch(data_dispatcher_t *dispatcher, data_dispatcher_type_t type, void *data) {
  assert(dispatcher != NULL);
  assert(data != NULL);

  fixed_queue_t *queue;
  auto iter = dispatcher->dispatch_table->find(type);
  if (iter == dispatcher->dispatch_table->end())
    queue = dispatcher->default_queue;
  else
    queue = iter->second;

  if (queue)
    fixed_queue_enqueue(queue, data);
  else
    LOG_WARN(LOG_TAG, "%s has no handler for type (%zd) in data dispatcher named: %s", __func__, type, dispatcher->name);

  return queue != NULL;
}