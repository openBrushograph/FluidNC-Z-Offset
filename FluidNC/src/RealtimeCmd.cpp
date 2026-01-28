#include "RealtimeCmd.h"
#include <cmath> // Added for round()
#include "Config.h"
#include "Channel.h"
#include "Protocol.h"
#include "Report.h"
#include "System.h"
#include "Machine/Macros.h"  // macroNEvent
#include "Machine/MachineConfig.h"

// Act upon a realtime character
void execute_realtime_command(Cmd command, Channel& channel) {
    switch (command) {
        case Cmd::Reset:
            protocol_send_event(&rtResetEvent);
            break;
        case Cmd::StatusReport:
            report_realtime_status(channel);  // direct call instead of setting flag
            // protocol_send_event(&reportStatusEvent, int(&channel));
            break;
        case Cmd::CycleStart:
            protocol_send_event(&cycleStartEvent);
            break;
        case Cmd::FeedHold:
            protocol_send_event(&feedHoldEvent);
            break;
        case Cmd::SafetyDoor:
            protocol_send_event(&safetyDoorEvent);
            break;
        case Cmd::JogCancel:
            if (state_is(State::Jog)) {  // Block all other states from invoking motion cancel.
                protocol_send_event(&motionCancelEvent);
            }
            break;
        case Cmd::DebugReport:
            protocol_send_event(&debugEvent);
            break;
        case Cmd::SpindleOvrStop:
            protocol_send_event(&accessoryOverrideEvent, AccessoryOverride::SpindleStopOvr);
            break;
        case Cmd::FeedOvrReset:
            protocol_send_event(&feedOverrideEvent, FeedOverride::Default);
            break;
        case Cmd::FeedOvrCoarsePlus:
            protocol_send_event(&feedOverrideEvent, FeedOverride::CoarseIncrement);
            break;
        case Cmd::FeedOvrCoarseMinus:
            protocol_send_event(&feedOverrideEvent, -FeedOverride::CoarseIncrement);
            break;
        case Cmd::FeedOvrFinePlus:
            protocol_send_event(&feedOverrideEvent, FeedOverride::FineIncrement);
            break;
        case Cmd::FeedOvrFineMinus:
            protocol_send_event(&feedOverrideEvent, -FeedOverride::FineIncrement);
            break;
        case Cmd::RapidOvrReset:
            protocol_send_event(&rapidOverrideEvent, RapidOverride::Default);
            break;
        case Cmd::RapidOvrMedium:
            protocol_send_event(&rapidOverrideEvent, RapidOverride::Medium);
            break;
        case Cmd::RapidOvrLow:
            protocol_send_event(&rapidOverrideEvent, RapidOverride::Low);
            break;
        case Cmd::RapidOvrExtraLow:
            protocol_send_event(&rapidOverrideEvent, RapidOverride::ExtraLow);
            break;
        case Cmd::SpindleOvrReset:
            protocol_send_event(&spindleOverrideEvent, SpindleSpeedOverride::Default);
            break;
        case Cmd::SpindleOvrCoarsePlus:
            protocol_send_event(&spindleOverrideEvent, SpindleSpeedOverride::CoarseIncrement);
            break;
        case Cmd::SpindleOvrCoarseMinus:
            protocol_send_event(&spindleOverrideEvent, -SpindleSpeedOverride::CoarseIncrement);
            break;
        case Cmd::SpindleOvrFinePlus:
            protocol_send_event(&spindleOverrideEvent, SpindleSpeedOverride::FineIncrement);
            break;
        case Cmd::SpindleOvrFineMinus:
            protocol_send_event(&spindleOverrideEvent, -SpindleSpeedOverride::FineIncrement);
            break;
        case Cmd::CoolantFloodOvrToggle:
            protocol_send_event(&accessoryOverrideEvent, AccessoryOverride::FloodToggle);
            break;
        case Cmd::CoolantMistOvrToggle:
            protocol_send_event(&accessoryOverrideEvent, AccessoryOverride::MistToggle);
            break;
        case Cmd::Macro0:
            protocol_send_event(&macro0Event);
            break;
        case Cmd::Macro1:
            protocol_send_event(&macro1Event);
            break;
        case Cmd::Macro2:
            protocol_send_event(&macro2Event);
            break;
        case Cmd::Macro3:
            protocol_send_event(&macro3Event);
            break;
// ... (cleanup)
        case Cmd::BabystepZUp: {
            float steps_mm = config->_axes->_axis[Z_AXIS]->_stepsPerMm;
            // Standard: 0.1mm
            int steps = round(steps_mm * 0.1f);
            if (steps < 1) steps = 1;
            protocol_send_event(&babystepEvent, (void*)steps);
            break;
        }
        case Cmd::BabystepZDown: {
            float steps_mm = config->_axes->_axis[Z_AXIS]->_stepsPerMm;
            // Standard: 0.1mm
            int steps = round(steps_mm * 0.1f);
            if (steps < 1) steps = 1;
            // Note: Protocol handler casts to int, so -steps is correct for direction
            protocol_send_event(&babystepEvent, (void*)-steps);
            break;
        }
        case Cmd::BabystepZUpFast: {
             float steps_mm = config->_axes->_axis[Z_AXIS]->_stepsPerMm;
             // Fast: 0.5mm
             int steps = round(steps_mm * 0.5f);
             if (steps < 1) steps = 1;
             protocol_send_event(&babystepEvent, (void*)steps);
             break;
         }
         case Cmd::BabystepZDownFast: {
             float steps_mm = config->_axes->_axis[Z_AXIS]->_stepsPerMm;
             // Fast: 0.5mm
             int steps = round(steps_mm * 0.5f);
             if (steps < 1) steps = 1;
             protocol_send_event(&babystepEvent, (void*)-steps);
             break;
         }
        default:
            break;
    }
}

// checks to see if a character is a realtime character
bool is_realtime_command(uint8_t data) {
    if (data >= 0x80) {
        return true;
    }
    auto cmd = static_cast<Cmd>(data);
    return cmd == Cmd::Reset || cmd == Cmd::StatusReport || cmd == Cmd::CycleStart || cmd == Cmd::FeedHold;
}
