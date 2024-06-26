/*
 *------------------------------------------------------------------
 * netmap_api.c - netmap api
 *
 * Copyright (c) 2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------
 */

#include <vnet/vnet.h>
#include <vlibmemory/api.h>

#include <vnet/interface.h>
#include <vnet/api_errno.h>
#include <netmap/netmap.h>

#include <vnet/format_fns.h>
#include <netmap/netmap.api_enum.h>
#include <netmap/netmap.api_types.h>

#include <vlibapi/api_helper_macros.h>

#define foreach_vpe_api_msg                             \
_(NETMAP_CREATE, netmap_create)                                         \
_(NETMAP_DELETE, netmap_delete)                                         \

static void
vl_api_netmap_create_t_handler (vl_api_netmap_create_t * mp)
{
  vlib_main_t *vm = vlib_get_main ();
  vl_api_netmap_create_reply_t *rmp;
  int rv = 0;
  u8 *if_name = NULL;

  if_name = format (0, "%s", mp->netmap_if_name);
  vec_add1 (if_name, 0);

  rv =
    netmap_create_if (vm, if_name, mp->use_random_hw_addr ? 0 : mp->hw_addr,
		      mp->is_pipe, mp->is_master, 0);

  vec_free (if_name);

  REPLY_MACRO (VL_API_NETMAP_CREATE_REPLY);
}

static void
vl_api_netmap_delete_t_handler (vl_api_netmap_delete_t * mp)
{
  vlib_main_t *vm = vlib_get_main ();
  vl_api_netmap_delete_reply_t *rmp;
  int rv = 0;
  u8 *if_name = NULL;

  if_name = format (0, "%s", mp->netmap_if_name);
  vec_add1 (if_name, 0);

  rv = netmap_delete_if (vm, if_name);

  vec_free (if_name);

  REPLY_MACRO (VL_API_NETMAP_DELETE_REPLY);
}

#include <netmap/netmap.api.c>
static clib_error_t *
netmap_api_hookup (vlib_main_t * vm)
{
  /*
   * Set up the (msg_name, crc, message-id) table
   */
  setup_message_id_table ();

  return 0;
}

VLIB_API_INIT_FUNCTION (netmap_api_hookup);

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
