#include <iostream>
#include <sstream>
#include "PcoCameraStateMachine.h"
#include "PcoCameraLinkController.h"

using namespace FrameReceiver;

PcoCameraState::PcoCameraState(PcoCameraLinkController* controller) :
    controller_(controller)
{
    EventDisconnect::custom_static_type_ptr("disconnect");
    EventConnect::custom_static_type_ptr("connect");
    EventArm::custom_static_type_ptr("arm");
    EventDisarm::custom_static_type_ptr("disarm");
    EventRecordStart::custom_static_type_ptr("start");
    EventRecordStop::custom_static_type_ptr("stop");
}

void PcoCameraState::execute_command(const char* command)
{
    std::string cmd_str(command);
    this->execute_command(cmd_str);
}

void PcoCameraState::execute_command(std::string& command)
{
    CommandType command_type = map_command_to_type(command);

    if (command_type == CommandUnknown)
    {
        std::stringstream ss;
        ss << "Unknown camera state transition command: " << command;
        throw(PcoCameraStateUnknownCommand(ss.str()));
    }
    this->execute_command(command_type);
}

void PcoCameraState::execute_command(PcoCameraState::CommandType command)
{
    switch (command)
    {
        case CommandConnect:
            process_event(EventConnect());
            break;
        case CommandDisconnect:
            process_event(EventDisconnect());
            break;
        case CommandArm:
            process_event(EventArm());
            break;
        case CommandDisarm:
            process_event(EventDisarm());
            break;
        case CommandStartRecording:
            process_event(EventRecordStart());
            break;
        case CommandStopRecording:
            process_event(EventRecordStop());
            break;
        case CommandUnknown:
            // Deliberate fall-through
        default:
            {
                std::stringstream ss;
                ss << "Unknown camera state transition command type: " << (int)command;
                throw(PcoCameraStateUnknownCommand(ss.str()));
            }
            break;
    }
}

void PcoCameraState::unconsumed_event(const sc::event_base& e)
{
    std::stringstream ss;
    ss << e.custom_dynamic_type_ptr<char>() << " is not valid in "
        << current_state_name() << " state";
    throw(PcoCameraStateIllegalTransition(ss.str()));
}

PcoCameraState::CommandType PcoCameraState::map_command_to_type(std::string& command)
{
    CommandType command_type = CommandUnknown;

    if (command_type_map_.size() == 0)
    {
        init_command_type_map();
    }

    if (command_type_map_.left.count(command))
    {
        command_type = command_type_map_.left.at(command);
    }
    return command_type;
}

std::string PcoCameraState::map_state_to_name(StateType state_type)
{
    std::string state_name = "unknown";

    if (state_type_map_.size() == 0)
    {
        init_state_type_map();
    }

    if (state_type_map_.right.count(state_type))
    {
        state_name = state_type_map_.right.at(state_type);
    }
    return state_name;
}

std::string PcoCameraState::current_state_name(void)
{
    return map_state_to_name(current_state());
}

PcoCameraState::StateType PcoCameraState::current_state(void)
{
    return state_cast<const IStateInfo&>().state_type();
}

void PcoCameraState::init_command_type_map(void)
{
    command_type_map_.insert(CommandTypeMapEntry("connect",    CommandConnect));
    command_type_map_.insert(CommandTypeMapEntry("disconnect", CommandDisconnect));
    command_type_map_.insert(CommandTypeMapEntry("arm",        CommandArm));
    command_type_map_.insert(CommandTypeMapEntry("disarm",     CommandDisarm));
    command_type_map_.insert(CommandTypeMapEntry("start",      CommandStartRecording));
    command_type_map_.insert(CommandTypeMapEntry("stop",       CommandStopRecording));
}

void PcoCameraState::init_state_type_map(void)
{
    state_type_map_.insert(StateTypeMapEntry("disconnected", StateDisconnected));
    state_type_map_.insert(StateTypeMapEntry("connected",    StateConnected));
    state_type_map_.insert(StateTypeMapEntry("armed",        StateArmed));
    state_type_map_.insert(StateTypeMapEntry("recording",    StateRecording));
}

sc::result Disconnected::react(const EventConnect&)
{
    outermost_context().controller_->connect();
    return transit<Connected>();
}

sc::result Connected::react(const EventDisconnect&)
{
    outermost_context().controller_->disconnect();
    return transit<Disconnected>();
}

sc::result Connected::react(const EventArm&)
{
    outermost_context().controller_->arm();
    return transit<Armed>();
}

sc::result Armed::react(const EventDisarm&)
{
    outermost_context().controller_->disarm();
    return transit<Connected>();
}

sc::result Armed::react(const EventRecordStart&)
{
    outermost_context().controller_->start_recording();
    return transit<Recording>();
}

sc::result Recording::react(const EventRecordStop&)
{
    outermost_context().controller_->stop_recording();
    return transit<Armed>();
}