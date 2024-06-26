/*
 * Copyright (c) 2015 Cisco and/or its affiliates.
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
 */
#ifndef __included_vat_h__
#define __included_vat_h__

#define _GNU_SOURCE
#include <stdio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <vppinfra/clib.h>
#include <vppinfra/format.h>
#include <vppinfra/error.h>
#include <vppinfra/elog.h>
#include <vppinfra/time.h>
#include <vppinfra/macros.h>
#include <vppinfra/socket.h>
#include <vnet/vnet.h>
#include <vlib/vlib.h>
#include <vlib/unix/unix.h>
#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vlibmemory/memclnt.api_enum.h>
#include <vlibmemory/memclnt.api_types.h>

#include "vat/json_format.h"

#include <vlib/vlib.h>

typedef struct
{
  u8 *interface_name;
  u32 sw_if_index;
  /*
   * Subinterface ID. A number 0-N to uniquely identify this
   * subinterface under the super interface
   */
  u32 sub_id;

  /* Number of tags 0-2 */
  u8 sub_number_of_tags;
  u16 sub_outer_vlan_id;
  u16 sub_inner_vlan_id;

  union
  {
    u16 raw_flags;
    struct
    {
      u16 no_tags:1;
      u16 one_tag:1;
      u16 two_tags:1;
      /* 0 = dot1q, 1=dot1ad */
      u16 sub_dot1ad:1;
      u16 sub_exact_match:1;
      u16 sub_default:1;
      u16 sub_outer_vlan_id_any:1;
      u16 sub_inner_vlan_id_any:1;
    };
  };

  /* vlan tag rewrite */
  u32 vtr_op;
  u32 vtr_push_dot1q;
  u32 vtr_tag1;
  u32 vtr_tag2;
} sw_interface_subif_t;

typedef struct
{
  u8 ip[16];
  u8 prefix_length;
} ip_address_details_t;

typedef struct
{
  u8 present;
  ip_address_details_t *addr;
} ip_details_t;

typedef struct
{
  u64 packets;
  u64 bytes;
} interface_counter_t;

typedef struct
{
  struct in_addr address;
  u8 address_length;
  u64 packets;
  u64 bytes;
} ip4_fib_counter_t;

typedef struct
{
  struct in6_addr address;
  u8 address_length;
  u64 packets;
  u64 bytes;
} ip6_fib_counter_t;

typedef struct
{
  struct in_addr address;
  vnet_link_t linkt;
  u64 packets;
  u64 bytes;
} 
ip4_nbr_counter_t;

typedef struct
{
  struct in6_addr address;
  vnet_link_t linkt;
  u64 packets;
  u64 bytes;
} ip6_nbr_counter_t;

struct vat_registered_features_t;

typedef struct vat_main_
{
  /* vpe input queue 输出队列*/
  svm_queue_t *vl_input_queue;

  /* interface name table 接口名称表*/
  uword *sw_if_index_by_interface_name;

  /* subinterface table 子接口表*/
  sw_interface_subif_t *sw_if_subif_table;

  /* Graph node table 图节点表*/
  uword *graph_node_index_by_name;
  vlib_node_t ***graph_nodes;

  /* ip tables */
  ip_details_t *ip_details_by_sw_if_index[2];

  /* sw_if_index of currently processed interface */
  u32 current_sw_if_index;

  /* remember that we are dumping ipv6 */
  u8 is_ipv6;

  /* function table */
  uword *function_by_name;

  /* help strings */
  uword *help_by_name;

  /* macro table */
  clib_macro_main_t macro_main;

  /* Errors by number */
  uword *error_string_by_error_number;

  /* Main thread can spin (w/ timeout) here if needed 
  如果需要，主线程可以在这里旋转（带超时）。*/
  u32 async_mode;
  u32 async_errors;
  volatile u32 result_ready;
  volatile i32 retval;
  volatile u32 sw_if_index;
  volatile u8 *shmem_result;
  u8 *cmd_reply;

  /* our client index */
  u32 my_client_index;
  int client_index_invalid;

  /* Time is of the essence... */
  clib_time_t clib_time;

  /* Unwind (so we can quit) */
  jmp_buf jump_buf;
  int jump_buf_set;
  volatile int do_exit;

  /* temporary parse buffer 临时解析缓冲区*/
  unformat_input_t *input;

  /* input buffer */
  u8 *inbuf;

  /* stdio input / output FILEs */
  FILE *ifp, *ofp;
  u8 *current_file;
  u32 input_line_number;

  /* exec mode toggle 执行模式切换*/
  int exec_mode;

  /* Regenerate the interface table 重新生成接口表*/
  volatile int regenerate_interface_table;

  /* flag for JSON output format */
  u8 json_output;

  /* flag for interface event display */
  u8 interface_event_display;

  /* JSON tree used in composing dump api call results 
  用于合成转储 api 调用结果的 JSON 树*/
  vat_json_node_t json_tree;

  /* counters */
  u64 **simple_interface_counters;
  interface_counter_t **combined_interface_counters;
  ip4_fib_counter_t **ip4_fib_counters;
  u32 *ip4_fib_counters_vrf_id_by_index;
  ip6_fib_counter_t **ip6_fib_counters;
  u32 *ip6_fib_counters_vrf_id_by_index;
  ip4_nbr_counter_t **ip4_nbr_counters;
  ip6_nbr_counter_t **ip6_nbr_counters;

  ssvm_private_t stat_segment;
  clib_spinlock_t *stat_segment_lockp;

  socket_client_main_t *socket_client_main;
  u8 *socket_name;

  elog_main_t elog_main;

  struct vat_registered_features_t *feature_function_registrations;

  int (*api_sw_interface_dump) (struct vat_main_ *);

  /* Convenience */
  vlib_main_t *vlib_main;
} vat_main_t;

extern vat_main_t vat_main;

void vat_suspend (vlib_main_t * vm, f64 interval);
f64 vat_time_now (vat_main_t * vam);
void errmsg (char *fmt, ...);
void vat_api_hookup (vat_main_t * vam);
int api_sw_interface_dump (vat_main_t * vam);
void do_one_file (vat_main_t * vam);
int exec (vat_main_t * vam);

/* Plugin API library functions */
extern char *vat_plugin_path;
extern char *vat_plugin_name_filter;
void vat_plugin_api_reference (void);
uword unformat_sw_if_index (unformat_input_t * input, va_list * args);
uword unformat_ip4_address (unformat_input_t * input, va_list * args);
uword unformat_ethernet_address (unformat_input_t * input, va_list * args);
uword unformat_ethernet_type_host_byte_order (unformat_input_t * input,
					      va_list * args);
uword unformat_ip6_address (unformat_input_t * input, va_list * args);
u8 *format_ip4_address (u8 * s, va_list * args);
u8 *format_ip6_address (u8 * s, va_list * args);
u8 *format_ip46_address (u8 * s, va_list * args);
u8 *format_ethernet_address (u8 * s, va_list * args);

int vat_socket_connect (vat_main_t * vam);

#if VPP_API_TEST_BUILTIN
#define print api_cli_output
void api_cli_output (void *, const char *fmt, ...);
#else
#define print fformat_append_cr
void fformat_append_cr (FILE *, const char *fmt, ...);
#endif


typedef clib_error_t *(vat_feature_function_t) (vat_main_t * vam);
typedef struct vat_registered_features_t
{
  vat_feature_function_t *function;
  struct vat_registered_features_t *next;
} vat_registered_features_t;


#define VAT_REGISTER_FEATURE_FUNCTION(x)                               \
    vat_registered_features_t _vat_feature_function_##x;               \
static void __vlib_add_config_function_##x (void)                      \
    __attribute__((__constructor__)) ;                                 \
static void __vlib_add_config_function_##x (void)                      \
{                                                                      \
  vat_main_t * vam = &vat_main;                                                \
  _vat_feature_function_##x.next = vam->feature_function_registrations;        \
  vam->feature_function_registrations = &_vat_feature_function_##x;    \
}                                                                      \
 vat_registered_features_t _vat_feature_function_##x =		       \
   {								       \
    .function = x,						       \
   }

#if VPP_API_TEST_BUILTIN == 0
static_always_inline uword
api_unformat_sw_if_index (unformat_input_t *input, va_list *args)
{
  vat_main_t *vam = va_arg (*args, vat_main_t *);
  u32 *result = va_arg (*args, u32 *);
  u8 *if_name;
  uword *p;

  if (!unformat (input, "%s", &if_name))
    return 0;

  p = hash_get_mem (vam->sw_if_index_by_interface_name, if_name);
  if (p == 0)
    return 0;
  *result = p[0];
  return 1;
}
#endif /* VPP_API_TEST_BUILTIN */

#endif /* __included_vat_h__ */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
