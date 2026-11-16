#include <stdio.h>
#include <string.h>
#include "vpi_user.h"
#include "include/pmem.h"

static long long g_inst = 0;

static uint32_t arg_u32(vpiHandle *it) {
  vpiHandle a = vpi_scan(*it);
  s_vpi_value v;
  v.format = vpiIntVal;
  vpi_get_value(a, &v);
  return (uint32_t)v.value.integer;
}

static PLI_INT32 pmem_read_calltf(PLI_BYTE8 *ud) {
  (void)ud;
  vpiHandle sys = vpi_handle(vpiSysTfCall, NULL);
  vpiHandle it  = vpi_iterate(vpiArgument, sys);
  uint32_t addr = arg_u32(&it);

  s_vpi_value rv;
  rv.format = vpiIntVal;
  rv.value.integer = (PLI_INT32)pmem_read(addr);
  vpi_put_value(sys, &rv, NULL, vpiNoDelay);
  return 0;
}

static PLI_INT32 pmem_write_calltf(PLI_BYTE8 *ud) {
  (void)ud;
  vpiHandle sys = vpi_handle(vpiSysTfCall, NULL);
  vpiHandle it  = vpi_iterate(vpiArgument, sys);
  uint32_t addr = arg_u32(&it);
  uint32_t data = arg_u32(&it);
  uint32_t mask = arg_u32(&it);
  pmem_write(addr, data, (char)mask);
  return 0;
}

static PLI_INT32 pmem_load_calltf(PLI_BYTE8 *ud) {
  (void)ud;
  vpiHandle sys = vpi_handle(vpiSysTfCall, NULL);
  vpiHandle it  = vpi_iterate(vpiArgument, sys);
  vpiHandle a   = vpi_scan(it);

  s_vpi_value v;
  v.format = vpiStringVal;
  if (a != NULL) vpi_get_value(a, &v);

  if (a == NULL || v.value.str == NULL || pmem_load(v.value.str) < 0) {
    vpi_printf("[VPI] $pmem_load: 加载失败 (检查 +img=<path>)\n");
    vpip_set_return_value(1);
    vpi_control(vpiFinish, 1);
  }
  return 0;
}

static PLI_INT32 sim_retire_calltf(PLI_BYTE8 *ud) {
  (void)ud;
  g_inst++;
  return 0;
}

static PLI_INT32 ebreak_calltf(PLI_BYTE8 *ud) {
  (void)ud;

  vpiHandle sys = vpi_handle(vpiSysTfCall, NULL);
  vpiHandle it = vpi_iterate(vpiArgument, sys);
  uint32_t code = arg_u32(&it);
  vpi_free_object(it);

  vpip_set_return_value(code == 0 ? 0 : 1);
  vpi_control(vpiFinish, 0);
  return 0;
}

static PLI_INT32 at_end(p_cb_data cb) {
  (void)cb;
  vpi_printf("[VPI] %lld instructions executed\n", g_inst);
  return 0;
}

static s_vpi_systf_data tf_list[] = {
  { .type = vpiSysFunc, .sysfunctype = vpiIntFunc, .tfname = "$pmem_read",  .calltf = pmem_read_calltf  },
  { .type = vpiSysTask,                            .tfname = "$pmem_write", .calltf = pmem_write_calltf },
  { .type = vpiSysTask,                            .tfname = "$pmem_load",  .calltf = pmem_load_calltf  },
  { .type = vpiSysTask,                            .tfname = "$sim_retire", .calltf = sim_retire_calltf },
  { .type = vpiSysTask,                            .tfname = "$ebreak", .calltf = ebreak_calltf },
};

static void register_all(void) {
  for (unsigned i = 0; i < sizeof(tf_list) / sizeof(tf_list[0]); i++)
    vpi_register_systf(&tf_list[i]);

  s_cb_data cb;
  memset(&cb, 0, sizeof cb);
  cb.reason = cbEndOfSimulation;
  cb.cb_rtn  = at_end;
  vpi_register_cb(&cb);
}

void (*vlog_startup_routines[])(void) = { register_all, 0 };
