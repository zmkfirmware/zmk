# Source after attaching to the left controller with its matching ELF.
set pagination off
set $key_tick = -1LL
set $display_tick = -1LL
set $rpc_tick = -1LL
set $sent_tick = -1LL
set $key_endpoint = -1

define corne-trace-ui
  hbreak *zmk_physical_layouts_kscan_process_msgq
  commands
    silent
    set $key_tick = curr_tick
    set $key_endpoint = current_instance.transport
    continue
  end
  hbreak *display_tick_cb
  commands
    silent
    set $display_tick = curr_tick
    continue
  end
end

define corne-trace-original
  corne-trace-ui
  hbreak *rpc_tx_buffer_write
  commands
    silent
    set $rpc_tick = curr_tick
    continue
  end
  watch -location rpc_tx_buf.get.tail
  commands
    silent
    set $sent_tick = curr_tick
    continue
  end
end

define corne-trace-fixed
  corne-trace-ui
  hbreak *studio_tx_send
  commands
    silent
    set $rpc_tick = curr_tick
    continue
  end
  hbreak *studio_tx_complete
  commands
    silent
    if (int)$r1 > 0
      set $sent_tick = curr_tick
    end
    continue
  end
end

define corne-snapshot
  printf "ticks now=%lld local_key_work=%lld display_work=%lld rpc_write=%lld tx_progress=%lld\n", (long long)curr_tick, $key_tick, $display_tick, $rpc_tick, $sent_tick
  printf "last local key endpoint=%d, current endpoint follows\n", (int)$key_endpoint
  print current_instance
  printf "nRF52840 POWER.RESETREAS\n"
  x/wx 0x40000400
  info registers pc lr sp xpsr
  bt
  print k_sys_work_q.thread.base.thread_state
  print k_sys_work_q.thread.callee_saved
end
