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
/*
 * node.h: VLIB processing nodes
 *
 * Copyright (c) 2008 Eliot Dresselhaus
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef included_vlib_node_h
#define included_vlib_node_h

#include <vppinfra/cpu.h>
#include <vppinfra/longjmp.h>
#include <vppinfra/lock.h>
#include <vlib/trace.h>		/* for vlib_trace_filter_t */

/* Forward declaration. */
struct vlib_node_runtime_t;
struct vlib_frame_t;

/* Internal nodes (including output nodes) move data from node to
   node (or out of the graph for output nodes). */
typedef uword (vlib_node_function_t) (struct vlib_main_t * vm,
				      struct vlib_node_runtime_t * node,
				      struct vlib_frame_t * frame);

typedef enum
{
  VLIB_NODE_PROTO_HINT_NONE = 0,
  VLIB_NODE_PROTO_HINT_ETHERNET,
  VLIB_NODE_PROTO_HINT_IP4,
  VLIB_NODE_PROTO_HINT_IP6,
  VLIB_NODE_PROTO_HINT_TCP,
  VLIB_NODE_PROTO_HINT_UDP,
  VLIB_NODE_N_PROTO_HINTS,
} vlib_node_proto_hint_t;

typedef enum
{
  /* An internal node on the call graph (could be output). */
  VLIB_NODE_TYPE_INTERNAL,

  /* Nodes which input data into the processing graph.
     Input nodes are called for each iteration of main loop. 主循环的每次迭代都会调用输入节点*/
  VLIB_NODE_TYPE_INPUT,

  /* Nodes to be called before all input nodes.
     Used, for example, to clean out driver TX rings before
     processing input. */
  VLIB_NODE_TYPE_PRE_INPUT,

  /* "Process" nodes which can be suspended and later resumed. */
  VLIB_NODE_TYPE_PROCESS,

  /* Nodes to by called by per-thread timing wheel. */
  VLIB_NODE_TYPE_SCHED,

  VLIB_N_NODE_TYPE,
} vlib_node_type_t;

typedef struct
{
  u8 can_be_disabled : 1;
  u8 may_receive_interrupts : 1;
  u8 decrement_main_loop_per_calls_if_polling : 1;
  u8 supports_adaptive_mode : 1;
  u8 can_be_polled : 1;
  u8 is_process : 1;
} vlib_node_type_atts_t;

static const vlib_node_type_atts_t node_type_attrs[VLIB_N_NODE_TYPE] ={
  [VLIB_NODE_TYPE_PRE_INPUT] = {
    .can_be_disabled = 1,
    .may_receive_interrupts = 1,
    .decrement_main_loop_per_calls_if_polling = 1,
    .can_be_polled = 1,
  },
  [VLIB_NODE_TYPE_INPUT] = {
    .can_be_disabled = 1,
    .may_receive_interrupts = 1,
    .decrement_main_loop_per_calls_if_polling = 1,
    .supports_adaptive_mode = 1,
    .can_be_polled = 1,
  },
  [VLIB_NODE_TYPE_PROCESS] = {
    .can_be_disabled = 1,
    .is_process = 1,
  },
  [VLIB_NODE_TYPE_SCHED] = {
    .can_be_disabled = 1,
    .may_receive_interrupts = 1,
  },
};

typedef struct _vlib_node_fn_registration
{
  vlib_node_function_t *function;
  clib_march_variant_type_t march_variant;
  struct _vlib_node_fn_registration *next_registration;
} vlib_node_fn_registration_t;

typedef struct _vlib_node_registration
{
  /* Vector processing function for this node. */
  vlib_node_function_t *function;

  /* Node function candidate registration with priority */
  vlib_node_fn_registration_t *node_fn_registrations;

  /* Node name. */
  char *name;

  /* Name of sibling (if applicable). */
  char *sibling_of;

  /* Node index filled in by registration. */
  u32 index;

  /* Type of this node. */
  vlib_node_type_t type;

  /* Error strings indexed by error code for this node. */
  char **error_strings;
  vlib_error_desc_t *error_counters;

  /* Buffer format/unformat for this node. */
  format_function_t *format_buffer;
  unformat_function_t *unformat_buffer;

  /* Trace format/unformat for this node. */
  format_function_t *format_trace;
  unformat_function_t *unformat_trace;

  /* Function to validate incoming frames. */
  u8 *(*validate_frame) (struct vlib_main_t * vm,
			 struct vlib_node_runtime_t *,
			 struct vlib_frame_t * f);

  /* Per-node runtime data. */
  void *runtime_data;

  /* Process stack size. */
  u16 process_log2_n_stack_bytes;

  /* Number of bytes of per-node run time data. */
  u8 runtime_data_bytes;

  /* State for input nodes. */
  u8 state;

  /* Node flags. */
  u16 flags;

  /* protocol at b->data[b->current_data] upon entry to the dispatch fn */
  u8 protocol_hint;

  /* Size of scalar and vector arguments in bytes. */
  u8 vector_size, aux_size;
  u16 scalar_size;

  /* Number of error codes used by this node. */
  u16 n_errors;

  /* Number of next node names that follow. */
  u16 n_next_nodes;

  /* Constructor link-list, don't ask... */
  struct _vlib_node_registration *next_registration;

  /* Names of next nodes which this node feeds into. */
  char *next_nodes[];

} vlib_node_registration_t;

#ifndef CLIB_MARCH_VARIANT
#define VLIB_REGISTER_NODE(x, ...)                                            \
  __VA_ARGS__ vlib_node_registration_t x;                                     \
  static void __vlib_add_node_registration_##x (void)                         \
    __attribute__ ((__constructor__));                                        \
  static void __vlib_add_node_registration_##x (void)                         \
  {                                                                           \
    vlib_global_main_t *vgm = vlib_get_global_main ();                        \
    x.next_registration = vgm->node_registrations;                            \
    vgm->node_registrations = &x;                                             \
  }                                                                           \
  static void __vlib_rm_node_registration_##x (void)                          \
    __attribute__ ((__destructor__));                                         \
  static void __vlib_rm_node_registration_##x (void)                          \
  {                                                                           \
    vlib_global_main_t *vgm = vlib_get_global_main ();                        \
    VLIB_REMOVE_FROM_LINKED_LIST (vgm->node_registrations, &x,                \
				  next_registration);                         \
  }                                                                           \
  __VA_ARGS__ vlib_node_registration_t x
#else
#define VLIB_REGISTER_NODE(x,...)                                       \
STATIC_ASSERT (sizeof(# __VA_ARGS__) != 7,"node " #x " must not be declared as static"); \
static __clib_unused vlib_node_registration_t __clib_unused_##x
#endif

#ifndef CLIB_MARCH_VARIANT
#define CLIB_MARCH_VARIANT_STR "default"
#else
#define _CLIB_MARCH_VARIANT_STR(s) __CLIB_MARCH_VARIANT_STR(s)
#define __CLIB_MARCH_VARIANT_STR(s) #s
#define CLIB_MARCH_VARIANT_STR _CLIB_MARCH_VARIANT_STR(CLIB_MARCH_VARIANT)
#endif

#define VLIB_NODE_FN(node)                                                    \
  uword CLIB_MARCH_SFX (node##_fn) (vlib_main_t *, vlib_node_runtime_t *,     \
				    vlib_frame_t *);                          \
  static vlib_node_fn_registration_t CLIB_MARCH_SFX (                         \
    node##_fn_registration) = {                                               \
    .function = &CLIB_MARCH_SFX (node##_fn),                                  \
  };                                                                          \
                                                                              \
  static void __clib_constructor CLIB_MARCH_SFX (node##_multiarch_register) ( \
    void)                                                                     \
  {                                                                           \
    extern vlib_node_registration_t node;                                     \
    vlib_node_fn_registration_t *r;                                           \
    r = &CLIB_MARCH_SFX (node##_fn_registration);                             \
    r->march_variant = CLIB_MARCH_SFX (CLIB_MARCH_VARIANT_TYPE);              \
    r->next_registration = node.node_fn_registrations;                        \
    node.node_fn_registrations = r;                                           \
  }                                                                           \
  uword CLIB_MARCH_SFX (node##_fn)

unformat_function_t unformat_vlib_node_variant;

typedef struct
{
  /* Total calls, clock ticks and vector elements processed for this node. */
  u64 calls, vectors, clocks, suspends;
  u64 max_clock;
  u64 max_clock_n;
} vlib_node_stats_t;

#define foreach_vlib_node_state					\
  /* Input node is called each iteration of main loop.		\
     This is the default (zero). */				\
  _ (POLLING)							\
  /* Input node is called when device signals an interrupt. */	\
  _ (INTERRUPT)							\
  /* Input node is never called. */				\
  _ (DISABLED)

typedef enum
{
#define _(f) VLIB_NODE_STATE_##f,
  foreach_vlib_node_state
#undef _
    VLIB_N_NODE_STATE,
} __clib_packed vlib_node_state_t;

typedef enum
{
  VLIB_NODE_DISPATCH_REASON_UNKNOWN = 0,
  VLIB_NODE_DISPATCH_REASON_PENDING_FRAME,
  VLIB_NODE_DISPATCH_REASON_POLL,
  VLIB_NODE_DISPATCH_REASON_INTERRUPT,
  VLIB_NODE_DISPATCH_REASON_SCHED,
  VLIB_NODE_DISPATCH_N_REASON,
} __clib_packed vlib_node_dispatch_reason_t;

#define vlib_node_dispatch_reason_enum_strings                                \
  {                                                                           \
    [VLIB_NODE_DISPATCH_REASON_UNKNOWN] = "unknown",                          \
    [VLIB_NODE_DISPATCH_REASON_PENDING_FRAME] = "pending-frame",              \
    [VLIB_NODE_DISPATCH_REASON_POLL] = "poll",                                \
    [VLIB_NODE_DISPATCH_REASON_INTERRUPT] = "interrupt",                      \
    [VLIB_NODE_DISPATCH_REASON_SCHED] = "scheduled",                          \
  }

typedef struct vlib_node_t
{
  /* Vector processing function for this node. 节点的矢量处理函数 */
  vlib_node_function_t *function;

  /* Node name. */
  u8 *name;

  /* Node name index in elog string table. elog字符串表的节点名索引*/
  u32 name_elog_string;

  /* Total statistics for this node. 节点的统计总数*/
  vlib_node_stats_t stats_total;

  /* Saved values as of last clear (or zero if never cleared).上次清零时的保存值（如果从未清零，则为零）。
     Current values are always stats_total - stats_last_clear. 当前值始终为 stats_total - stats_last_clear。*/
  vlib_node_stats_t stats_last_clear;

  /* Type of this node. 节点类型*/
  vlib_node_type_t type;

  /* Node index. */
  u32 index;

  /* Index of corresponding node runtime. 相应节点运行时的索引*/
  u32 runtime_index;

  /* Runtime data for this node. 节点运行时的诗句*/
  u8 *runtime_data;

  /* Node flags. 节点标记*/
  u16 flags;

  /* Processing function keeps frame.  Tells node dispatching code not
     to free frame after dispatch is done.  
     处理函数保留帧。  告诉节点调度代码在调度完成后不要释放帧。*/
#define VLIB_NODE_FLAG_FRAME_NO_FREE_AFTER_DISPATCH (1 << 0)

  /* Node counts as output/drop/punt node for stats purposes.
  就统计而言，节点算作输出/删除/分流节点 */
#define VLIB_NODE_FLAG_IS_OUTPUT (1 << 1)
#define VLIB_NODE_FLAG_IS_DROP (1 << 2)
#define VLIB_NODE_FLAG_IS_PUNT (1 << 3)
#define VLIB_NODE_FLAG_IS_HANDOFF (1 << 4)

  /* Set if current node runtime has traced vectors.
  设置当前节点运行时是否跟踪矢量 */
#define VLIB_NODE_FLAG_TRACE (1 << 5)

#define VLIB_NODE_FLAG_SWITCH_FROM_INTERRUPT_TO_POLLING_MODE (1 << 6)
#define VLIB_NODE_FLAG_SWITCH_FROM_POLLING_TO_INTERRUPT_MODE (1 << 7)
#define VLIB_NODE_FLAG_TRACE_SUPPORTED (1 << 8)
#define VLIB_NODE_FLAG_ADAPTIVE_MODE			     (1 << 9)
#define VLIB_NODE_FLAG_ALLOW_LAZY_NEXT_NODES		     (1 << 10)

  /* State for input nodes. */
  u8 state;

  /* Number of bytes of run time data. */
  u8 runtime_data_bytes;

  /* protocol at b->data[b->current_data] upon entry to the dispatch fn 
  进入调度 fn 时，在 b->data[b->current_data] 处的协议*/
  u8 protocol_hint;

  /* Number of error codes used by this node. */
  u16 n_errors;

  /* Size of scalar and vector arguments in bytes. 标量和向量参数的大小（字节）*/
  u16 frame_size, scalar_offset, vector_offset, magic_offset, aux_offset;
  u16 frame_size_index;

  /* Handle/index in error heap for this node. 该节点在错误堆中的句柄/索引。*/
  u32 error_heap_handle;
  u32 error_heap_index;

  /* Counter structures indexed by counter code for this node. 该节点在错误堆中的句柄/索引。*/
  vlib_error_desc_t *error_counters;

  /* Vector of next node names.下一个节点名向量
     Only used before next_nodes array is initialized. */
  char **next_node_names;

  /* Next node indices for this node. */
  u32 *next_nodes;

  /* Name of node that we are sibling of. 兄弟节点的名称*/
  char *sibling_of;

  /* Bitmap of all of this node's siblings. */
  uword *sibling_bitmap;

  /* Total number of vectors sent to each next node.发送到下一个节点的向量总数。 */
  u64 *n_vectors_by_next_node;

  /* Hash table mapping next node index into slot in
     next_nodes vector.  Quickly determines whether this node
     is connected to given next node and, if so, with which slot. 
     将下一节点索引映射到 next_nodes 向量中插槽的哈希表。   
     快速确定此节点是否与给定的下一节点相连，如果是，则确定与哪个槽相连。*/
  uword *next_slot_by_node;

  /* Bitmap of node indices which feed this node. 
  为该节点提供数据的节点索引位图。*/
  uword *prev_node_bitmap;

  /* Node/next-index which own enqueue rights with to this node. 
  Node/next-index 拥有该节点的入队权限。*/
  u32 owner_node_index, owner_next_index;

  /* Buffer format/unformat for this node. */
  format_function_t *format_buffer;
  unformat_function_t *unformat_buffer;

  /* Trace buffer format/unformat for this node. */
  format_function_t *format_trace;

  /* Function to validate incoming frames. 
  验证传入帧的功能*/
  u8 *(*validate_frame) (struct vlib_main_t * vm,
			 struct vlib_node_runtime_t *,
			 struct vlib_frame_t * f);
  /* for pretty-printing, not typically valid */
  u8 *state_string;

  /* Node function candidate registration with priority 
  节点功能候选优先注册*/
  vlib_node_fn_registration_t *node_fn_registrations;
} vlib_node_t;

#define VLIB_INVALID_NODE_INDEX ((u32) ~0)

/* Max number of vector elements to process at once per node. */
#define VLIB_FRAME_SIZE 256
/* Number of extra elements allocated at the end of vecttor. */
#define VLIB_FRAME_SIZE_EXTRA 4
/* Frame data alignment */
#define VLIB_FRAME_DATA_ALIGN 16

/* Calling frame (think stack frame) for a node. */
typedef struct vlib_frame_t
{
  /* Frame flags. */
  u16 frame_flags;

  /* User flags. Used for sending hints to the next node. */
  u16 flags;

  /* Scalar, vector and aux offsets in this frame. */
  u16 scalar_offset, vector_offset, aux_offset;

  /* Number of vector elements currently in frame. */
  u16 n_vectors;

  /* Index of frame size corresponding to allocated node. */
  u16 frame_size_index;

  /* Scalar and vector arguments to next node. */
  u8 arguments[0];
} vlib_frame_t;

typedef struct
{
  /* Frame pointer. */
  vlib_frame_t *frame;

  /* Node runtime for this next. */
  u32 node_runtime_index;

  /* Next frame flags. */
  u32 flags;

  /* Reflects node frame-used flag for this next. */
#define VLIB_FRAME_NO_FREE_AFTER_DISPATCH \
  VLIB_NODE_FLAG_FRAME_NO_FREE_AFTER_DISPATCH

  /* Don't append this frame */
#define VLIB_FRAME_NO_APPEND (1 << 14)

  /* This next frame owns enqueue to node
     corresponding to node_runtime_index. */
#define VLIB_FRAME_OWNER (1 << 15)

  /* Set when frame has been allocated for this next. */
#define VLIB_FRAME_IS_ALLOCATED	VLIB_NODE_FLAG_IS_OUTPUT

  /* Set when frame has been added to pending vector. */
#define VLIB_FRAME_PENDING VLIB_NODE_FLAG_IS_DROP

  /* Set when frame is to be freed after dispatch. */
#define VLIB_FRAME_FREE_AFTER_DISPATCH VLIB_NODE_FLAG_IS_PUNT

  /* Set when frame has traced packets. */
#define VLIB_FRAME_TRACE VLIB_NODE_FLAG_TRACE

  /* Number of vectors enqueue to this next since last overflow. */
  u32 vectors_since_last_overflow;
} vlib_next_frame_t;

always_inline void
vlib_next_frame_init (vlib_next_frame_t * nf)
{
  clib_memset (nf, 0, sizeof (nf[0]));
  nf->node_runtime_index = ~0;
}

/* A frame pending dispatch by main loop. */
typedef struct
{
  /* Frame index (in the heap). */
  vlib_frame_t *frame;

  /* Node and runtime for this frame. */
  u32 node_runtime_index;

  /* Start of next frames for this node. */
  u32 next_frame_index;

  /* Special value for next_frame_index when there is no next frame. */
#define VLIB_PENDING_FRAME_NO_NEXT_FRAME ((u32) ~0)
} vlib_pending_frame_t;

typedef struct vlib_node_runtime_t
{
  CLIB_CACHE_LINE_ALIGN_MARK (cacheline0);	/**< cacheline mark */

  vlib_node_function_t *function;	/**< Node function to call. */

  vlib_error_t *errors;			/**< Vector of errors for this node. */

  u32 clocks_since_last_overflow;	/**< Number of clock cycles. */

  u32 max_clock;			/**< Maximum clock cycle for an
					  invocation. */

  u32 max_clock_n;			/**< Number of vectors in the recorded
					  max_clock. */

  u32 calls_since_last_overflow;	/**< Number of calls. */

  u32 vectors_since_last_overflow;	/**< Number of vector elements
					  processed by this node. */

  u32 next_frame_index;			/**< Start of next frames for this
					  node. */

  u32 node_index;			/**< Node index. */

  u32 input_main_loops_per_call;	/**< For input nodes: decremented
					  on each main loop interation until
					  it reaches zero and function is
					  called.  Allows some input nodes to
					  be called more than others. */

  u32 main_loop_count_last_dispatch;	/**< Saved main loop counter of last
					  dispatch of this node. */

  u32 main_loop_vector_stats[2];

  u16 flags;				/**< Copy of main node flags. */

  vlib_node_state_t state; /**< Input node state. */

  vlib_node_dispatch_reason_t
    dispatch_reason; /**< Reason for running this node. */

  u16 n_next_nodes;

  u16 cached_next_index;		/**< Next frame index that vector
					  arguments were last enqueued to
					  last time this node ran. Set to
					  zero before first run of this
					  node. */
  u32 stop_timer_handle_plus_1;		/**< Timing wheel stop handle for
					  SCHED node incremented by 1,
					  0 = no timer running. */

  CLIB_ALIGN_MARK (runtime_data_pad, 8);

  u8 runtime_data[0];			/**< Function dependent
					  node-runtime data. This data is
					  thread local, and it is not
					  cloned from main thread. It needs
					  to be initialized for each thread
					  before it is used unless
					  runtime_data template exists in
					  vlib_node_t. */
}
vlib_node_runtime_t;

#define VLIB_NODE_RUNTIME_DATA_SIZE	(sizeof (vlib_node_runtime_t) - STRUCT_OFFSET_OF (vlib_node_runtime_t, runtime_data))

typedef struct
{
  /* Number of allocated frames for this scalar/vector size. */
  u32 n_alloc_frames;

  /* Frame size */
  u16 frame_size;

  /* Vector of free frames for this scalar/vector size. */
  vlib_frame_t **free_frames;
} vlib_frame_size_t;

STATIC_ASSERT_SIZEOF (vlib_frame_size_t, 16);

typedef struct
{
  /* Users opaque value for event type. */
  uword opaque;
} vlib_process_event_type_t;

#define foreach_vlib_process_state                                            \
  _ (NOT_STARTED, "not started")                                              \
  _ (RUNNING, "running")                                                      \
  _ (SUSPENDED, "suspended")                                                  \
  _ (WAIT_FOR_EVENT, "event wait")                                            \
  _ (WAIT_FOR_CLOCK, "clock wait")                                            \
  _ (WAIT_FOR_EVENT_OR_CLOCK, "any wait")                                     \
  _ (WAIT_FOR_ONE_TIME_EVENT, "one time event wait")                          \
  _ (YIELD, "yield")

typedef enum
{
#define _(n, s) VLIB_PROCESS_STATE_##n,
  foreach_vlib_process_state VLIB_PROCESS_N_STATES
#undef _
} __clib_packed vlib_process_state_t;

STATIC_ASSERT (VLIB_PROCESS_STATE_NOT_STARTED == 0, "");

typedef enum
{
  VLIB_PROCESS_RESTORE_REASON_UNKNOWN = 0,
  VLIB_PROCESS_RESTORE_REASON_EVENT,
  VLIB_PROCESS_RESTORE_REASON_CLOCK,
  VLIB_PROCESS_RESTORE_REASON_TIMED_EVENT,
  VLIB_PROCESS_RESTORE_REASON_YIELD,
  VLIB_PROCRSS_N_RESTORE_REASON
} __clib_packed vlib_process_restore_reason_t;

typedef struct
{
  vlib_process_restore_reason_t reason;
  union
  {
    u32 runtime_index;
    u32 timed_event_data_pool_index;
  };
} __clib_packed vlib_process_restore_t;

typedef struct
{
  CLIB_CACHE_LINE_ALIGN_MARK (cacheline0);
  /* Node runtime for this process. */
  vlib_node_runtime_t node_runtime;

  /* Where to longjmp when process is done. */
  clib_longjmp_t return_longjmp;

#define VLIB_PROCESS_RETURN_LONGJMP_RETURN ((uword) ~0 - 0)
#define VLIB_PROCESS_RETURN_LONGJMP_SUSPEND ((uword) ~0 - 1)

  /* Where to longjmp to resume node after suspend. */
  clib_longjmp_t resume_longjmp;
#define VLIB_PROCESS_RESUME_LONGJMP_SUSPEND 0
#define VLIB_PROCESS_RESUME_LONGJMP_RESUME  1

  /* Process state. */
  vlib_process_state_t state;

  /* Process is added to resume list due to pending event  */
  u8 event_resume_pending : 1;

  /* Size of process stack. */
  u16 log2_n_stack_bytes;

  u32 suspended_process_frame_index;

  /* Number of times this process was suspended. */
  u32 n_suspends;

  /* Vectors of pending event data indexed by event type index. */
  u8 **pending_event_data_by_type_index;

  /* Bitmap of event type-indices with non-empty vectors. */
  uword *non_empty_event_type_bitmap;

  /* Bitmap of event type-indices which are one time events. */
  uword *one_time_event_type_bitmap;

  /* Type is opaque pointer -- typically a pointer to an event handler
     function.  Hash table to map opaque to a type index. */
  uword *event_type_index_by_type_opaque;

  /* Pool of currently valid event types. */
  vlib_process_event_type_t *event_type_pool;

  /*
   * When suspending saves clock time (10us ticks) when process
   * is to be resumed.
   */
  u64 resume_clock_interval;

  /* Handle from timer code, to cancel an unexpired timer */
  u32 stop_timer_handle;

  /* Default output function and its argument for any CLI outputs
     within the process. */
  vlib_cli_output_function_t *output_function;
  uword output_function_arg;

  /* Process stack */
#define VLIB_PROCESS_STACK_MAGIC (0xdead7ead)
  u32 *stack;
} vlib_process_t;

typedef struct
{
  u32 node_index;

  u32 one_time_event;
} vlib_one_time_waiting_process_t;

typedef struct
{
  u16 n_data_elts;

  u16 n_data_elt_bytes;

  /* n_data_elts * n_data_elt_bytes */
  u32 n_data_bytes;

  /* Process node & event type to be used to signal event. */
  u32 process_node_index;

  u32 event_type_index;

  union
  {
    u8 inline_event_data[64 - 3 * sizeof (u32) - 2 * sizeof (u16)];

    /* Vector of event data used only when data does not fit inline. */
    u8 *event_data_as_vector;
  };
}
vlib_signal_timed_event_data_t;

typedef struct
{
  clib_march_variant_type_t index;
  int priority;
  char *suffix;
  char *desc;
} vlib_node_fn_variant_t;

typedef struct
{
  /* Public nodes. */
  vlib_node_t **nodes;

  /* Node index hashed by node name. */
  uword *node_by_name;

  u32 flags;
#define VLIB_NODE_MAIN_RUNTIME_STARTED (1 << 0)

  /* Nodes segregated by type for cache locality.
     Does not apply to nodes of type VLIB_NODE_TYPE_INTERNAL. */
  vlib_node_runtime_t *nodes_by_type[VLIB_N_NODE_TYPE];

  /* Node runtime indices for input nodes with pending interrupts. */
  void *node_interrupts[VLIB_N_NODE_TYPE];

  /* Input nodes are switched from/to interrupt to/from polling mode
     when average vector length goes above/below polling/interrupt
     thresholds. */
  u32 polling_threshold_vector_length;
  u32 interrupt_threshold_vector_length;

  /* Vector of next frames. */
  vlib_next_frame_t *next_frames;

  /* Vector of internal node's frames waiting to be called. */
  vlib_pending_frame_t *pending_frames;

  vlib_signal_timed_event_data_t *signal_timed_event_data_pool;

  /* Vector of process nodes waiting for restore */
  vlib_process_restore_t *process_restore_current;

  /* Vector of sched nodes waiting to be calleed */
  u32 *sched_node_pending;

  /* Vector of process nodes waiting for restore in next greaph scheduler run
   */
  vlib_process_restore_t *process_restore_next;

  /* CPU time of next process to be ready on timing wheel. */
  f64 time_next_process_ready;

  /* Vector of process nodes.
     One for each node of type VLIB_NODE_TYPE_PROCESS. */
  vlib_process_t **processes;

  /* Current running process or ~0 if no process running. */
  u32 current_process_index;

  /* Pool of pending process frames. */
  vlib_pending_frame_t *suspended_process_frames;

  /* Vector of event data vectors pending recycle. */
  void **recycled_event_data_vectors;

  /* Current counts of nodes in each state. */
  u32 input_node_counts_by_state[VLIB_N_NODE_STATE];

  /* Per-size frame allocation information. */
  vlib_frame_size_t *frame_sizes;

  /* Time of last node runtime stats clear. */
  f64 time_last_runtime_stats_clear;

  /* Node index from error code */
  u32 *node_by_error;

  /* Node Function Variants */
  vlib_node_fn_variant_t *variants;

  /* Node Function Default Variant Index */
  u32 node_fn_default_march_variant;

  /* Node Function march Variant by Suffix Hash */
  uword *node_fn_march_variant_by_suffix;
} vlib_node_main_t;

typedef u16 vlib_error_t;

always_inline u32
vlib_error_get_node (vlib_node_main_t * nm, vlib_error_t e)
{
  return nm->node_by_error[e];
}

always_inline u32
vlib_error_get_code (vlib_node_main_t * nm, vlib_error_t e)
{
  u32 node_index = nm->node_by_error[e];
  vlib_node_t *n = nm->nodes[node_index];
  u32 error_code = e - n->error_heap_index;
  return error_code;
}

#define FRAME_QUEUE_MAX_NELTS 64
typedef struct
{
  CLIB_CACHE_LINE_ALIGN_MARK (cacheline0);
  u64 head;
  u64 tail;
  u32 n_in_use;
  u32 nelts;
  u32 written;
  u32 threshold;
  i32 n_vectors[FRAME_QUEUE_MAX_NELTS];
} frame_queue_trace_t;

typedef struct
{
  u64 count[FRAME_QUEUE_MAX_NELTS];
} frame_queue_nelt_counter_t;

#endif /* included_vlib_node_h */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
