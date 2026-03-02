flowchart TB
  %% ============================================================
  %% PREN Puzzlebot – minimal file-level architecture
  %% (ONLY files essential for robot runtime)
  %% ============================================================

  subgraph TOP["source/"]
    main_c["main.c\n(superloop)"]
    platform_h["platform.h\n(common includes)"]
  end

  subgraph CFG["source/config/"]
    robot_cfg_h["robot_config.h\n(macros / tuning)"]
  end

  subgraph PROTO["source/proto/"]
    cmd_c["cmd.c\ncmd_poll()\nparse + dispatch"]
    cmd_h["cmd.h"]
    protocol_h["protocol.h\nTypes + g_status\n(robot_pos_s, bot_action_s, err_e)"]
    protocol_c["protocol.c\nstatus/reply helpers\n(g_status owner definitions)"]
    serial_port_h["serial_port.h\nserial abstraction"]
    serial_port_tiny_c["serial_port_tinyk22.c\nserial impl via UART0"]
  end

  subgraph COM["source/com/"]
    uart_h["uart.h\nUART API"]
    uart0_c["uart0.c\nUART0 driver"]
  end

  subgraph CTRL["source/controls/"]
    bot_c["bot.c\nAction FIFO + orchestration\nbot_step(), bot_enqueue()"]
    bot_h["bot.h"]
    job_c["job.c\nJob sequencing\nsegments + (optional) correction"]
    job_h["job.h"]
  end

  subgraph MOT["source/motion/"]
    motion_c["motion.c\nExecutes ONE segment\nmotion_start(), motion_is_done()\nupdates pos_cmd"]
    motion_h["motion.h"]
  end

  subgraph POS["source/position/"]
    position_c["position.c\nEncoder -> pos_measured\nposition_poll(), set offset"]
    position_h["position.h"]
  end

  subgraph IO["source/io/"]
    io_c["io.c\nGPIO abstraction\nSTEP/DIR/EN, endstops, misc"]
    io_h["io.h"]
  end

  subgraph TICK["source/utils/"]
    ftm3_c["ftm3.c\nstepper tick base\nftm3_tick_init/start"]
    ftm3_h["ftm3.h"]
  end

  %% ============================================================
  %% Superloop (top layer)
  %% ============================================================
  main_c -->|"init subsystems"| io_c
  main_c -->|"init subsystems"| motion_c
  main_c -->|"init subsystems"| position_c
  main_c -->|"init subsystems"| bot_c
  main_c -->|"init subsystems"| job_c
  main_c -->|"tick init/start\n(precise stepping)"| ftm3_c

  main_c -->|"superloop:\ncmd_poll() (UART RX)"| cmd_c
  main_c -->|"superloop:\nbot_step() (dequeue actions)"| bot_c
  main_c -->|"superloop:\njob_step() (segment sequencing)"| job_c
  main_c -->|"superloop:\nposition_poll() (encoder update)"| position_c

  %% ============================================================
  %% Command input path
  %% ============================================================
  cmd_c -->|"RX/TX bytes\nvia serial_port API"| serial_port_tiny_c
  serial_port_tiny_c -->|"UART0 bytes"| uart0_c
  uart0_c --> uart_h

  cmd_c -->|"build bot_action_s\n{type=ACT_MOVE/HOME/..,\n target_pos, flags}\nENQUEUE"| bot_c

  %% ============================================================
  %% Orchestration + job start
  %% ============================================================
  bot_c -->|"Action FIFO\nstores bot_action_s"| bot_c
  bot_c -->|"dequeue action\njob_start_*()"| job_c

  %% ============================================================
  %% Job -> Motion segments (execution boundary)
  %% ============================================================
  job_c -->|"motion_start(segment)\nINPUT: target_pos / segment params"| motion_c
  job_c -->|"motion_is_done(), motion_last_err()"| motion_c

  %% Optional: closed-loop correction boundary
  job_c -->|"READ g_status.pos_measured\ncompare to final_target\nplan correction segment"| protocol_h
  job_c -->|"may call position_set_xy_mm_scaled()\n(after homing to set 0)"| position_c

  %% ============================================================
  %% Motion execution internals
  %% ============================================================
  motion_c -->|"STEP/DIR/EN signals"| io_c
  motion_c -->|"uses tick base\n(FTM3 ISR drives step timing)"| ftm3_c
  motion_c -->|"WRITE g_status.pos_cmd\n(model pos from steps)"| protocol_h
  motion_c -->|"READ macros\nsteps/mm, limits, speed"| robot_cfg_h

  %% ============================================================
  %% Position feedback internals
  %% ============================================================
  position_c -->|"WRITE g_status.pos_measured\n(sensor pos from encoder)"| protocol_h
  position_c -->|"READ macros\ncounts/mm, invert, toggles"| robot_cfg_h

  %% ============================================================
  %% IO boundaries
  %% ============================================================
  io_c -->|"endstop reads used by homing (job/motion)"| job_c

  %% ============================================================
  %% Shared protocol state (global interface)
  %% ============================================================
  protocol_h --> cmd_c
  protocol_h --> bot_c
  protocol_h --> job_c
  protocol_h --> motion_c
  protocol_h --> position_c
  protocol_c -->|"format replies / status\n(uses g_status)"| cmd_c

  %% ============================================================
  %% Platform umbrella include
  %% ============================================================
  platform_h --> main_c
  platform_h --> cmd_c
  platform_h --> bot_c
  platform_h --> job_c
  platform_h --> motion_c
  platform_h --> position_c
  platform_h --> io_c
  platform_h --> protocol_c
  platform_h --> uart0_c
  platform_h --> serial_port_tiny_c