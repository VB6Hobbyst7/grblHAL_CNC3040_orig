/*
  report.c - reporting and messaging methods
  Part of Grbl

  Copyright (c) 2017-2018 Terje Io
  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
  This file functions as the primary feedback interface for Grbl. Any outgoing data, such
  as the protocol status messages, feedback messages, and status reports, are stored here.
  For the most part, these functions primarily are called from protocol.c methods. If a
  different style feedback is desired (i.e. JSON), then a user can change these following
  methods to accomodate their needs.
*/

#include "grbl.h"

// Internal report utilities to reduce flash with repetitive tasks turned into functions.
static void report_util_setting_prefix (setting_type_t n) {
    hal.serial_write('$');
    print_uint8_base10((uint8_t)n);
    hal.serial_write('=');
}

inline static void report_util_line_feed () {
    hal.serial_write_string("\r\n");
}

inline static void report_util_feedback_line_feed () {
    hal.serial_write_string("]\r\n");
}

inline static void report_util_gcode_modes_G () {
    hal.serial_write_string(" G");
}

inline static void report_util_gcode_modes_M () {
    hal.serial_write_string(" M");
}

// static void report_util_comment_line_feed() { hal.serial_write(')'); report_util_line_feed(); }

static void report_util_axis_values (float *axis_value) {
    uint_fast8_t idx;
    for (idx = 0; idx < N_AXIS; idx++) {
        printFloat_CoordValue(axis_value[idx]);
        if (idx < (N_AXIS - 1))
            hal.serial_write(',');
    }
}

void report_util_uint_setting (setting_type_t n, uint32_t val) {
    report_util_setting_prefix(n);
    print_uint32_base10(val);
    report_util_line_feed();
}

void report_util_float_setting (setting_type_t n, float val, uint8_t n_decimal) {
    report_util_setting_prefix(n);
    printFloat(val,n_decimal);
    report_util_line_feed();
}


// Handles the primary confirmation protocol response for streaming interfaces and human-feedback.
// For every incoming line, this method responds with an 'ok' for a successful command or an
// 'error:'  to indicate some error event with the line or some critical system error during
// operation. Errors events can originate from the g-code parser, settings module, or asynchronously
// from a critical error, such as a triggered hard limit. Interface should always monitor for these
// responses.
void report_status_message (status_code_t status_code)
{
    switch(status_code) {

        case Status_OK: // STATUS_OK
            hal.serial_write_string("ok\r\n"); break;

        default:
            hal.serial_write_string("error:");
            print_uint8_base10((uint8_t)status_code);
            report_util_line_feed();
    }
}

// Prints alarm messages.
void report_alarm_message (alarm_code_t alarm_code)
{
    hal.serial_write_string("ALARM:");
    print_uint8_base10((uint8_t)alarm_code);
    report_util_line_feed();
    hal.delay_ms(500, 0); // Force delay to ensure message clears serial write buffer.
}

// Prints feedback messages. This serves as a centralized method to provide additional
// user feedback for things that are not of the status/alarm message protocol. These are
// messages such as setup warnings, switch toggling, and how to exit alarms.
// NOTE: For interfaces, messages are always placed within brackets. And if silent mode
// is installed, the message number codes are less than zero.
void report_feedback_message(message_code_t message_code)
{
    hal.serial_write_string("[MSG:");

    switch(message_code) {

        case Message_CriticalEvent:
            hal.serial_write_string("Reset to continue");
            break;

        case Message_AlarmLock:
            hal.serial_write_string("'$H'|'$X' to unlock");
            break;

        case Message_AlarmUnlock:
            hal.serial_write_string("Caution: Unlocked");
            break;

        case Message_Enabled:
            hal.serial_write_string("Enabled");
            break;

        case Message_Disabled:
            hal.serial_write_string("Disabled");
            break;

        case Message_SafetyDoorAjar:
            hal.serial_write_string("Check Door");
            break;

        case Message_CheckLimits:
            hal.serial_write_string("Check Limits");
            break;

        case Message_ProgramEnd:
            hal.serial_write_string("Pgm End");
            break;

        case Message_RestoreDefaults:
            hal.serial_write_string("Restoring defaults");
            break;

        case Message_SpindleRestore:
            hal.serial_write_string("Restoring spindle");
            break;

        case Message_SleepMode:
            hal.serial_write_string("Sleeping");
            break;

        case Message_EStop:
            hal.serial_write_string("Emergency stop");
            break;

        default:
            break;
    }
    report_util_feedback_line_feed();
}


// Welcome message
void report_init_message ()
{
    hal.serial_write_string("\r\nGrblHAL " GRBL_VERSION " ['$' for help]\r\n");
}

// Grbl help message
void report_grbl_help () {
    hal.serial_write_string("[HLP:$$ $# $G $I $N $x=val $Nx=line $J=line $SLP $C $X $H $B ~ ! ? ctrl-x]\r\n");
}


// Grbl global settings print out.
// NOTE: The numbering scheme here must correlate to storing in settings.c
void report_grbl_settings() {

    // Print Grbl settings.
    report_util_uint_setting(Setting_PulseMicroseconds, settings.pulse_microseconds);
    report_util_uint_setting(Setting_StepperIdleLockTime, settings.stepper_idle_lock_time);
    report_util_uint_setting(Setting_StepInvertMask, settings.step_invert.mask);
    report_util_uint_setting(Setting_DirInvertMask, settings.dir_invert.mask);
    report_util_uint_setting(Setting_InvertStepperEnable, settings.stepper_enable_invert.mask);
    report_util_uint_setting(Setting_LimitPinsInvertMask, settings.limit_invert.mask);
    report_util_uint_setting(Setting_InvertProbePin, settings.flags.invert_probe_pin);
    report_util_uint_setting(Setting_StatusReportMask, settings.status_report.mask);
    report_util_float_setting(Setting_JunctionDeviation, settings.junction_deviation, N_DECIMAL_SETTINGVALUE);
    report_util_float_setting(Setting_ArcTolerance, settings.arc_tolerance, N_DECIMAL_SETTINGVALUE);
    report_util_uint_setting(Setting_ReportInches, settings.flags.report_inches);
    report_util_uint_setting(Setting_ControlInvertMask, settings.control_invert.mask);
    report_util_uint_setting(Setting_CoolantInvertMask, settings.coolant_invert.mask);
    report_util_uint_setting(Setting_SpindleInvertMask, settings.spindle_invert.mask);
    report_util_uint_setting(Setting_ControlPullUpDisableMask, settings.control_disable_pullup.mask);
    report_util_uint_setting(Setting_LimitPullUpDisableMask, settings.limit_disable_pullup.mask);
    report_util_uint_setting(Setting_ProbePullUpDisable, settings.flags.disable_probe_pullup);
    report_util_uint_setting(Setting_SoftLimitsEnable, settings.flags.soft_limit_enable);
    report_util_uint_setting(Setting_HardLimitsEnable, settings.flags.hard_limit_enable);
    report_util_uint_setting(Setting_HomingEnable, settings.flags.homing_enable);
    report_util_uint_setting(Setting_HomingDirMask, settings.homing_dir_mask);
    report_util_float_setting(Setting_HomingFeedRate, settings.homing_feed_rate, N_DECIMAL_SETTINGVALUE);
    report_util_float_setting(Setting_HomingSeekRate, settings.homing_seek_rate, N_DECIMAL_SETTINGVALUE);
    report_util_uint_setting(Setting_HomingDebounceDelay, settings.homing_debounce_delay);
    report_util_float_setting(Setting_HomingPulloff, settings.homing_pulloff, N_DECIMAL_SETTINGVALUE);
    report_util_float_setting(Setting_G73Retract, settings.g73_retract, N_DECIMAL_SETTINGVALUE);
    report_util_uint_setting(Setting_PulseDelayMicroseconds, settings.pulse_delay_microseconds);
    report_util_float_setting(Setting_RpmMax, settings.rpm_max, N_DECIMAL_RPMVALUE);
    report_util_float_setting(Setting_RpmMin, settings.rpm_min, N_DECIMAL_RPMVALUE);
    report_util_uint_setting(Setting_LaserMode, hal.driver_cap.variable_spindle ? settings.flags.laser_mode : 0);
    report_util_float_setting(Setting_PWMFreq, settings.spindle_pwm_freq, N_DECIMAL_SETTINGVALUE);
    report_util_float_setting(Setting_PWMOffValue, settings.spindle_pwm_off_value, N_DECIMAL_SETTINGVALUE);
    report_util_float_setting(Setting_PWMMinValue, settings.spindle_pwm_min_value, N_DECIMAL_SETTINGVALUE);
    report_util_float_setting(Setting_PWMMaxValue, settings.spindle_pwm_max_value, N_DECIMAL_SETTINGVALUE);
    report_util_uint_setting(Setting_StepperDeenergizeMask, settings.stepper_deenergize.mask);
    if(hal.driver_cap.spindle_sync) {
        report_util_uint_setting(Setting_SpindlePPR, settings.spindle_ppr);
        report_util_float_setting(Setting_SpindlePGain, settings.spindle_P_gain, N_DECIMAL_SETTINGVALUE);
        report_util_float_setting(Setting_SpindleIGain, settings.spindle_I_gain, N_DECIMAL_SETTINGVALUE);
        report_util_float_setting(Setting_SpindleDGain, settings.spindle_D_gain, N_DECIMAL_SETTINGVALUE);
    }
    report_util_uint_setting(Setting_HomingLocateCycles, settings.homing_locate_cycles);

    uint_fast8_t idx;

    for(idx = 0 ; idx < N_AXIS ; idx++)
        report_util_uint_setting((setting_type_t)(Setting_HomingCycle_1 + idx), settings.homing_cycle[idx]);

    if(hal.driver_settings_report)
        hal.driver_settings_report(false);

    // Print axis settings
    uint_fast8_t set_idx, val = (uint_fast8_t)Setting_AxisSettingsBase;
    for (set_idx = 0; set_idx < AXIS_N_SETTINGS; set_idx++) {

        for (idx = 0; idx < N_AXIS; idx++) {

            switch ((axis_setting_type_t)set_idx) {

                case AxisSetting_StepsPerMM:
                    report_util_float_setting((setting_type_t)(val + idx), settings.steps_per_mm[idx], N_DECIMAL_SETTINGVALUE);
                    break;

                case AxisSetting_MaxRate:
                    report_util_float_setting((setting_type_t)(val + idx), settings.max_rate[idx], N_DECIMAL_SETTINGVALUE);
                    break;

                case AxisSetting_Acceleration:
                    report_util_float_setting((setting_type_t)(val + idx), settings.acceleration[idx] / (60.0f * 60.0f), N_DECIMAL_SETTINGVALUE);
                    break;

                case AxisSetting_MaxTravel:
                    report_util_float_setting((setting_type_t)(val + idx), -settings.max_travel[idx], N_DECIMAL_SETTINGVALUE);
                    break;

              #if AXIS_N_SETTINGS > 4
                case AxisSetting_StepperCurrent:
                    report_util_float_setting((setting_type_t)(val + idx), settings.current[idx], N_DECIMAL_SETTINGVALUE);
                    break;
              #endif

                default: // for stopping compiler warning
                    break;
            }
        }
        val += AXIS_SETTINGS_INCREMENT;
    }

    if(hal.driver_settings_report)
        hal.driver_settings_report(true);
}


// Prints current probe parameters. Upon a probe command, these parameters are updated upon a
// successful probe or upon a failed probe with the G38.3 without errors command (if supported).
// These values are retained until Grbl is power-cycled, whereby they will be re-zeroed.
void report_probe_parameters ()
{
    // Report in terms of machine position.
    hal.serial_write_string("[PRB:");
    float print_position[N_AXIS];
    system_convert_array_steps_to_mpos(print_position, sys_probe_position);
    report_util_axis_values(print_position);
    hal.serial_write(':');
    print_uint8_base10(sys.probe_succeeded);
    report_util_feedback_line_feed();
}


// Prints Grbl NGC parameters (coordinate offsets, probing, tool table)
void report_ngc_parameters ()
{
    float coord_data[N_AXIS];
    uint_fast8_t idx, g5x;

    for (idx = 0; idx < SETTING_INDEX_NCOORD; idx++) {

        if (!(settings_read_coord_data(idx, &coord_data))) {
            report_status_message(Status_SettingReadFail);
            return;
        }

        hal.serial_write_string("[G");

        switch (idx) {

            case SETTING_INDEX_G28:
                hal.serial_write_string("28");
                break;
            case SETTING_INDEX_G30:
                hal.serial_write_string("30");
                break;
            default:
                g5x = idx + 54;
                print_uint8_base10(g5x > 59 ? 59 : g5x);
                if(g5x > 59) {
                    hal.serial_write_string(".");
                    print_uint8_base10(g5x - 59);
                }
                break; // G54-G59
        }
        hal.serial_write(':');
        report_util_axis_values(coord_data);
        report_util_feedback_line_feed();
    }

    hal.serial_write_string("[G92:"); // Print G92,G92.1 which are not persistent in memory
    report_util_axis_values(gc_state.g92_coord_offset);
    report_util_feedback_line_feed();
#ifdef N_TOOLS
    for (idx = 1; idx <= N_TOOLS; idx++) {
        hal.serial_write_string("[T");
        print_uint8_base10(idx);
        hal.serial_write(':');
        report_util_axis_values(tool_table[idx].offset);
        report_util_feedback_line_feed();
    }
#endif
    hal.serial_write_string("[TLO:"); // Print tool length offset value
    report_util_axis_values(gc_state.tool_length_offset);
    report_util_feedback_line_feed();

    report_probe_parameters(); // Print probe parameters. Not persistent in memory.
}


// Print current gcode parser mode state
void report_gcode_modes ()
{
    hal.serial_write_string("[GC:G");
    if (gc_state.modal.motion >= MotionMode_ProbeToward) {
        hal.serial_write_string("38.");
        print_uint8_base10(gc_state.modal.motion - (MotionMode_ProbeToward - 2));
    } else
        print_uint8_base10(gc_state.modal.motion);

    uint8_t g5x = gc_state.modal.coord_system.idx + 54;
    report_util_gcode_modes_G();
    print_uint8_base10(g5x > 59 ? 59 : g5x);
    if(g5x > 59) {
        hal.serial_write_string(".");
        print_uint8_base10(g5x - 59);
    }

    report_util_gcode_modes_G();
    print_uint8_base10(gc_state.diameter_mode ? 7 : 8);

    report_util_gcode_modes_G();
    print_uint8_base10(gc_state.modal.plane_select + 17);

    report_util_gcode_modes_G();
    print_uint8_base10(21 - gc_state.modal.units);

    report_util_gcode_modes_G();
    print_uint8_base10(gc_state.modal.distance + 90);

    report_util_gcode_modes_G();
    print_uint8_base10(94 - gc_state.modal.feed_mode);

    report_util_gcode_modes_G();
    print_uint8_base10(gc_state.modal.scaling_active ? 51 : 50);

    if(gc_state.modal.scaling_active) {
        hal.serial_write(':');
        print_uint8_base10(gc_get_g51_state());

/* report using axis letters instead?
        g5x = gc_get_g50_status();

        uint_fast8_t idx = N_AXIS;
        for(idx = 0; idx < N_AXIS; idx++) {
            if(g5x & 0x01)
                hal.serial_write("XYZABC"[idx]);
            g5x >>= 1;
        }
*/
    }

    if (gc_state.modal.program_flow) {
        report_util_gcode_modes_M();
        switch (gc_state.modal.program_flow) {

            case ProgramFlow_Paused:
                hal.serial_write('0');
                break;

  /*            case PROGRAM_FLOW_OPTIONAL_STOP: // M1 is ignored and not supported.
                hal.serial_write('1');
                break; */

            case ProgramFlow_CompletedM2:
            case ProgramFlow_CompletedM30:
                print_uint8_base10((uint8_t)gc_state.modal.program_flow);
                break;

            default:
                break;
        }
    }

    report_util_gcode_modes_M();
    hal.serial_write(gc_state.modal.spindle.on ? (gc_state.modal.spindle.ccw ? '4' : '3') : '5');

    if(gc_state.tool_change) {
        report_util_gcode_modes_M();
        hal.serial_write('6');
    }

    if (gc_state.modal.coolant.value) {
        if (gc_state.modal.coolant.mist) {
             report_util_gcode_modes_M();
             hal.serial_write('7');
        }
        if (gc_state.modal.coolant.flood) {
            report_util_gcode_modes_M();
            hal.serial_write('8');
        }
    } else {
        report_util_gcode_modes_M();
        hal.serial_write('9');
    }

    if (sys.override_ctrl.feed_rate_disable) {
        report_util_gcode_modes_M();
        hal.serial_write_string("50");
    }

    if (sys.override_ctrl.spindle_rpm_disable) {
        report_util_gcode_modes_M();
        hal.serial_write_string("51");
    }

    if (sys.override_ctrl.feed_hold_disable) {
        report_util_gcode_modes_M();
        hal.serial_write_string("53");
    }

#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
    if (sys.override_ctrl.parking_disable) {
        report_util_gcode_modes_M();
        hal.serial_write_string("56");
    }
#endif

    hal.serial_write_string(" T");
    print_uint8_base10(gc_state.tool->tool);

    hal.serial_write_string(" F");
    printFloat_RateValue(gc_state.feed_rate);

    if(hal.driver_cap.variable_spindle) {
        hal.serial_write_string(" S");
        printFloat(gc_state.spindle.rpm, N_DECIMAL_RPMVALUE);
    }
    report_util_feedback_line_feed();
}

// Prints specified startup line
void report_startup_line (uint8_t n, char *line)
{
    hal.serial_write_string("$N");
    print_uint8_base10(n);
    hal.serial_write('=');
    hal.serial_write_string(line);
    report_util_line_feed();
}

void report_execute_startup_message (char *line, status_code_t status_code)
{
    hal.serial_write('>');
    hal.serial_write_string(line);
    hal.serial_write(':');
    report_status_message(status_code);
}

// Prints build info line
void report_build_info(char *line)
{
    hal.serial_write_string("[VER:" GRBL_VERSION "(");
    hal.serial_write_string(hal.info ? hal.info : "HAL");
    hal.serial_write_string(")." GRBL_VERSION_BUILD ":");
    hal.serial_write_string(line);
    report_util_feedback_line_feed();
    hal.serial_write_string("[OPT:"); // Generate compile-time build option list
    if(hal.driver_cap.variable_spindle)
        hal.serial_write('V');
    hal.serial_write('N');
    if(hal.driver_cap.mist_control)
        hal.serial_write('M');
  #ifdef COREXY
    hal.serial_write('C');
  #endif
  #ifdef PARKING_ENABLE
    hal.serial_write('P');
  #endif
  #ifdef HOMING_FORCE_SET_ORIGIN
    hal.serial_write('Z');
  #endif
  #ifdef HOMING_SINGLE_AXIS_COMMANDS
    hal.serial_write('H');
  #endif
  #ifdef LIMITS_TWO_SWITCHES_ON_AXES
    hal.serial_write('T');
  #endif
  #ifdef ALLOW_FEED_OVERRIDE_DURING_PROBE_CYCLES
    hal.serial_write('A');
  #endif
  #ifdef SPINDLE_ENABLE_OFF_WITH_ZERO_SPEED
    hal.serial_write('0');
  #endif
    if(hal.driver_cap.software_debounce)
        hal.serial_write('S');
  #ifdef ENABLE_PARKING_OVERRIDE_CONTROL
    hal.serial_write('R');
  #endif
  #ifndef HOMING_INIT_LOCK
    hal.serial_write('L');
  #endif
  #ifdef ENABLE_SAFETY_DOOR_INPUT_PIN
    hal.serial_write('+');
  #endif
  #ifndef ENABLE_RESTORE_EEPROM_WIPE_ALL // NOTE: Shown when disabled.
    hal.serial_write('*');
  #endif
  #ifndef ENABLE_RESTORE_EEPROM_DEFAULT_SETTINGS // NOTE: Shown when disabled.
    hal.serial_write('$');
  #endif
  #ifndef ENABLE_RESTORE_EEPROM_CLEAR_PARAMETERS // NOTE: Shown when disabled.
    hal.serial_write('#');
  #endif
  #ifndef ENABLE_BUILD_INFO_WRITE_COMMAND // NOTE: Shown when disabled.
    hal.serial_write('I');
  #endif
  #ifndef FORCE_BUFFER_SYNC_DURING_WCO_CHANGE // NOTE: Shown when disabled.
    hal.serial_write('W');
  #endif
  #ifdef N_TOOLS
    hal.serial_write('V'); // ATC supported
  #else
    if(hal.serial_suspend_read)
        hal.serial_write('U'); // Manual tool change supported (M6)
  #endif

  // NOTE: Compiled values, like override increments/max/min values, may be added at some point later.
  hal.serial_write(',');
  print_uint8_base10(BLOCK_BUFFER_SIZE - 1);
  hal.serial_write(',');
  print_uint32_base10(hal.rx_buffer_size);
  hal.serial_write(',');
  print_uint8_base10(N_AXIS);
#ifdef N_TOOLS
  hal.serial_write(',');
  print_uint8_base10(N_TOOLS);
#endif

  report_util_feedback_line_feed();
}


// Prints the character string line Grbl has received from the user, which has been pre-parsed,
// and has been sent into protocol_execute_line() routine to be executed by Grbl.
void report_echo_line_received (char *line)
{
    hal.serial_write_string("[echo: ");
    hal.serial_write_string(line);
    report_util_feedback_line_feed();
}


 // Prints real-time data. This function grabs a real-time snapshot of the stepper subprogram
 // and the actual location of the CNC machine. Users may change the following function to their
 // specific needs, but the desired real-time data report must be as short as possible. This is
 // requires as it minimizes the computational overhead and allows grbl to keep running smoothly,
 // especially during g-code programs with fast, short line segments and high frequency reports (5-20Hz).
void report_realtime_status ()
{
    int32_t current_position[N_AXIS]; // Copy current state of the system position variable
    float print_position[N_AXIS];

    memcpy(current_position, sys_position, sizeof(sys_position));
    system_convert_array_steps_to_mpos(print_position, current_position);

    // Report current machine state and sub-states
    hal.serial_write('<');

    switch (sys.state) {

        case STATE_IDLE:
            hal.serial_write_string("Idle");
            break;

        case STATE_CYCLE:
            hal.serial_write_string("Run");
            break;

        case STATE_HOLD:
            hal.serial_write_string("Hold:");
            print_uint8_base10((uint8_t)sys.holding_state - 1);
            break;

        case STATE_JOG:
            hal.serial_write_string("Jog");
            break;

        case STATE_HOMING:
            hal.serial_write_string("Home");
            break;

        case STATE_ESTOP:
        case STATE_ALARM:
            hal.serial_write_string("Alarm");
            break;

        case STATE_CHECK_MODE:
            hal.serial_write_string("Check");
            break;

        case STATE_SAFETY_DOOR:
            hal.serial_write_string("Door:");
            print_uint8_base10((uint8_t)sys.parking_state);
            break;

        case STATE_SLEEP:
            hal.serial_write_string("Sleep");
            break;

        case STATE_TOOL_CHANGE:
            hal.serial_write_string("Tool");
            break;

    }

    uint_fast8_t idx;
    float wco[N_AXIS];
    if (!settings.status_report.position_type || sys.report.wco_counter == 0) {
        for (idx = 0; idx < N_AXIS; idx++) {
            // Apply work coordinate offsets and tool length offset to current position.
            wco[idx] = gc_state.modal.coord_system.xyz[idx] + gc_state.g92_coord_offset[idx] + gc_state.tool_length_offset[idx];
            if (!settings.status_report.position_type)
                print_position[idx] -= wco[idx];
        }
    }

    // Report machine position
    if (settings.status_report.position_type)
        hal.serial_write_string("|MPos:");
    else
        hal.serial_write_string("|WPos:");

    report_util_axis_values(print_position);

    // Returns planner and serial read buffer states.

    if (settings.status_report.buffer_state) {
        hal.serial_write_string("|Bf:");
        print_uint8_base10(plan_get_block_buffer_available());
        hal.serial_write(',');
        print_uint32_base10(hal.serial_get_rx_buffer_available());
    }

    if(settings.status_report.line_numbers) {
        // Report current line number
        plan_block_t *cur_block = plan_get_current_block();
        if (cur_block != NULL && cur_block->line_number > 0) {
            hal.serial_write_string("|Ln:");
            print_uint32_base10((uint32_t)cur_block->line_number);
        }
    }

    // Report realtime feed speed
    if(settings.status_report.feed_speed) {
        if(hal.driver_cap.variable_spindle) {
            hal.serial_write_string("|FS:");
            printFloat_RateValue(st_get_realtime_rate());
            hal.serial_write(',');
            printFloat(sys.spindle_rpm, N_DECIMAL_RPMVALUE);
            if(hal.spindle_get_data /* && sys.mpg_mode */) {
                hal.serial_write(',');
                printFloat(hal.spindle_get_data(SpindleData_RPM).rpm, N_DECIMAL_RPMVALUE);
            }
        } else {
            hal.serial_write_string("|F:");
            printFloat_RateValue(st_get_realtime_rate());
        }
    }

    if(settings.status_report.pin_state) {

        axes_signals_t lim_pin_state = (axes_signals_t)hal.limits_get_state();
        control_signals_t ctrl_pin_state = hal.system_control_get_state();
        bool prb_pin_state = hal.probe_get_state();

        if (lim_pin_state.value | ctrl_pin_state.value | prb_pin_state | sys.block_delete_enabled) {

            hal.serial_write_string("|Pn:");

            if (prb_pin_state)
                hal.serial_write('P');

            if (lim_pin_state.value) {
                if(lim_pin_state.x)
                    hal.serial_write('X');
                if(lim_pin_state.y)
                    hal.serial_write('Y');
                if (lim_pin_state.z)
                    hal.serial_write('Z');
              #ifdef A_AXIS
                if (lim_pin_state.a)
                    hal.serial_write('A');
              #endif
              #ifdef B_AXIS
                if (lim_pin_state.b)
                    hal.serial_write('B');
              #endif
              #ifdef C_AXIS
                if (lim_pin_state.c)
                    hal.serial_write('C');
              #endif
            }

            if (ctrl_pin_state.value) {
                if (ctrl_pin_state.safety_door_ajar)
                    hal.serial_write('D');
                if (ctrl_pin_state.reset)
                    hal.serial_write('R');
                if (ctrl_pin_state.feed_hold)
                    hal.serial_write('H');
                if (ctrl_pin_state.cycle_start)
                    hal.serial_write('S');
                if (ctrl_pin_state.e_stop)
                    hal.serial_write('E');
                if (ctrl_pin_state.block_delete)
                    hal.serial_write('B');
                if (ctrl_pin_state.stop_disable)
                    hal.serial_write('T');
            }

            if(sys.block_delete_enabled)
                hal.serial_write('B');
        }
    }

    bool report_overrides = sys.report.ovr_counter <= 0;

    if(settings.status_report.work_coord_offset) {

        if (sys.report.wco_counter > 0)
            sys.report.wco_counter--;
        else {
            sys.report.wco_counter = sys.state & (STATE_HOMING|STATE_CYCLE|STATE_HOLD|STATE_JOG|STATE_SAFETY_DOOR)
                                      ? (REPORT_WCO_REFRESH_BUSY_COUNT - 1) // Reset counter for slow refresh
                                      : (REPORT_WCO_REFRESH_IDLE_COUNT - 1);
            report_overrides = false; // Set override on next report.
            hal.serial_write_string("|WCO:");
            report_util_axis_values(wco);
        }
    }

    if(settings.status_report.overrrides) {

        if (sys.report.ovr_counter > 0)
            sys.report.ovr_counter--;
        else if(report_overrides) {

            hal.serial_write_string("|Ov:");
            print_uint8_base10(sys.f_override);
            hal.serial_write(',');
            print_uint8_base10(sys.r_override);
            hal.serial_write(',');
            print_uint8_base10(sys.spindle_rpm_ovr);

            spindle_state_t sp_state = hal.spindle_get_state();
            coolant_state_t cl_state = hal.coolant_get_state();
            if (sp_state.on || cl_state.value || gc_state.tool_change || sys.report.ovr_counter < 0) {

                hal.serial_write_string("|A:");

                if (sp_state.on)
                    hal.serial_write(sp_state.ccw ? 'C' : 'S');

                if (cl_state.flood)
                    hal.serial_write('F');

                if (cl_state.mist)
                    hal.serial_write('M');

                if(gc_state.tool_change)
                    hal.serial_write('T');

            }

            sys.report.ovr_counter = sys.state & (STATE_HOMING|STATE_CYCLE|STATE_HOLD|STATE_JOG|STATE_SAFETY_DOOR)
                                      ? (REPORT_OVR_REFRESH_BUSY_COUNT - 1) // Reset counter for slow refresh
                                      : (REPORT_OVR_REFRESH_IDLE_COUNT - 1);

        }
    } else if(gc_state.tool_change)
        hal.serial_write_string("|A:T");

    if(sys.report.scaling) {
        hal.serial_write_string("|Sc:");
        print_uint8_base10(gc_get_g51_state());
        sys.report.scaling = false;
    }

    if(sys.report.mpg_mode) {
        hal.serial_write_string("|MPG:");
        hal.serial_write(sys.mpg_mode ? '1' : '0');
        sys.report.mpg_mode = false;
    }

    if(hal.userdefined_rt_report)
        hal.userdefined_rt_report();

    hal.serial_write('>');
    report_util_line_feed();
}

#ifdef PID_LOG
void report_pid_log (void)
{
    uint_fast16_t idx = 0;

    hal.serial_write_string("[PID:");
    printFloat(sys.pid_log.setpoint, N_DECIMAL_PIDVALUE);
    hal.serial_write(',');
    printFloat(sys.pid_log.t_sample, N_DECIMAL_PIDVALUE);
    hal.serial_write_string(",2|"); // 2 is number of values per sample!

    if(sys.pid_log.idx) do {
        printFloat(sys.pid_log.target[idx], N_DECIMAL_PIDVALUE);
        hal.serial_write(',');
        printFloat(sys.pid_log.actual[idx], N_DECIMAL_PIDVALUE);
        idx++;
        if(idx != sys.pid_log.idx)
            hal.serial_write(',');
    } while(idx != sys.pid_log.idx);

    report_util_feedback_line_feed();
}
#endif
