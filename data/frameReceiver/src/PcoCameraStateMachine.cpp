/*
 * PcoCameraStateMachine.cpp - finite state machine implementation for the PCO camera controller
 *
 * This class implements a finite state machine handing for the PCO camera controller. This
 * is based on the boost StartChart library. The camera state responds to known commands
 * passed to the controller by a client and reacts accordingly, executing state transitions and
 * triggering operations in the controller.
 *
 * Created on: 22 Sept 2021
 *     Author: Tim Nicholls, STFC Detector Systems Group
 */

#include <iostream>
#include <sstream>
#include "PcoCameraStateMachine.h"
#include "PcoCameraLinkController.h"

using namespace FrameReceiver;

//! Constructor for the PcoCameraState class.
//!
//! This constructor initialises the state machine, defining custom type information for the
//! transition events.
//!
//! \param controller - pointer to the PCO controller instance, which acts on state transitions

PcoCameraState::PcoCameraState(PcoCameraLinkController* controller) :
    controller_(controller)
{
    // Define custom type information for naming state transition events
    EventDisconnect::custom_static_type_ptr("disconnect");
    EventConnect::custom_static_type_ptr("connect");
    EventArm::custom_static_type_ptr("arm");
    EventDisarm::custom_static_type_ptr("disarm");
    EventRecordStart::custom_static_type_ptr("start");
    EventRecordStop::custom_static_type_ptr("stop");
}

//! Executes a state transition command.
//!
//! This method executes the specified state transition command. Specified as a pointer to a char
//! array, the command name is converted to a string and passed to the overloaded execute_command
//! method.
//!
//! \param command - command name as a character array pointer

void PcoCameraState::execute_command(const char* command)
{
    std::string cmd_str(command);
    this->execute_command(cmd_str);
}

//! Executes a state transition command.
//!
//! This method executes the specified state transition command. Specified as a pointer to a char
//! array, the command name is mapped to the command type value and passed to the overloaded
//! execute_command method.
//!
//! \param command - command name as string reference

void PcoCameraState::execute_command(std::string& command)
{
    // Map the specified command to a type value
    CommandType command_type = map_command_to_type(command);

    // If the command is not recognised, throw an exception
    if (command_type == CommandUnknown)
    {
        std::stringstream ss;
        ss << "Unknown camera state transition command: " << command;
        throw(PcoCameraStateUnknownCommand(ss.str()));
    }

    // Otherwise execute the command
    this->execute_command(command_type);
}

//! Executes a state transition command.
//!
//! This method executes the specified state transition command. THe command type specified as an
//! argument is mapped to appropriate transition event, which is then procssed.
//!
//! \param command - state transition command type valuet

void PcoCameraState::execute_command(PcoCameraState::CommandType command)
{
    // Acquire the transition lock to prevent concurrent access
    boost::lock_guard<boost::mutex> transition_lock(state_transition_mutex_);

    // Process the appropriate transition event based on the command value
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
                // If an illegal or unknown command value is specified, throw an exception
                std::stringstream ss;
                ss << "Unknown camera state transition command type: " << (int)command;
                throw(PcoCameraStateUnknownCommand(ss.str()));
            }
            break;
    }
}

//! Handles unconsumed, i.e. invalid, state transition events.
//!
//! This method is called when a state transition event is not consumed by the state machine, that
//! is when an transition is requested that is not valid in the current state. An illegal transition
//! exception is formatted with a relevent error string and thrown.
//!
//! \param event - reference to the unconsumed event

void PcoCameraState::unconsumed_event(const sc::event_base& event)
{
    // Format an appropriate error message
    std::stringstream ss;
    ss << event.custom_dynamic_type_ptr<char>() << " is not valid in "
        << current_state_name() << " state";

    // Throw an illegal transition exception
    throw(PcoCameraStateIllegalTransition(ss.str()));
}

//! Maps a command name string to a command type value
//!
//! This method maps a specified command name string to the appropriate command type, handling
//! unknown strings by returning a value of CommandUnknown.
//!
//! \param command - reference to the command name as a string
//! \return enumerated command type value

PcoCameraState::CommandType PcoCameraState::map_command_to_type(std::string& command)
{
    // Set default return value
    CommandType command_type = CommandUnknown;

    // Initialise the command type map if empty
    if (command_type_map_.size() == 0)
    {
        init_command_type_map();
    }

    // If the specified command is present in the map, set the command type accordingly
    if (command_type_map_.left.count(command))
    {
        command_type = command_type_map_.left.at(command);
    }
    return command_type;
}

//! Maps a state type to a state name.
//!
//! This methos maps an enumerated state type to a state name. Unknown state types return a
//! string name of unknown.
//!
//! \param state_type - state type value
//! \return state name as a string

std::string PcoCameraState::map_state_to_name(StateType state_type)
{
    // Set default return value
    std::string state_name = "unknown";

    // Initialise the command state map if empty
    if (state_type_map_.size() == 0)
    {
        init_state_type_map();
    }

    // If the specified state is present in the map, set the state name accordingly
    if (state_type_map_.right.count(state_type))
    {
        state_name = state_type_map_.right.at(state_type);
    }
    return state_name;
}

// Returns the current state name.
//!
//! This method returns the current state name as a string.
//!
//! \return current state name as a string

std::string PcoCameraState::current_state_name(void)
{
    return map_state_to_name(current_state());
}

//! Returns the current state type.
//!
//! This method returns the current state type value by calling the state_type interface method
//! on the current state object.
//!
//! \return current state type value

PcoCameraState::StateType PcoCameraState::current_state(void)
{
    return state_cast<const IStateInfo&>().state_type();
}

//! Initialises the command type map.
//!
//! This method initialises the entries in the command type map, which maps command names to
//! enumerated command type values.

void PcoCameraState::init_command_type_map(void)
{
    command_type_map_.insert(CommandTypeMapEntry("connect",    CommandConnect));
    command_type_map_.insert(CommandTypeMapEntry("disconnect", CommandDisconnect));
    command_type_map_.insert(CommandTypeMapEntry("arm",        CommandArm));
    command_type_map_.insert(CommandTypeMapEntry("disarm",     CommandDisarm));
    command_type_map_.insert(CommandTypeMapEntry("start",      CommandStartRecording));
    command_type_map_.insert(CommandTypeMapEntry("stop",       CommandStopRecording));
}

//! Initialises the state type map.
//!
//! This method initialises the entries in the state type map, which maps state names to
//! enumerated state type values.

void PcoCameraState::init_state_type_map(void)
{
    state_type_map_.insert(StateTypeMapEntry("disconnected", StateDisconnected));
    state_type_map_.insert(StateTypeMapEntry("connected",    StateConnected));
    state_type_map_.insert(StateTypeMapEntry("armed",        StateArmed));
    state_type_map_.insert(StateTypeMapEntry("recording",    StateRecording));
}

//! Reacts to connect transition events
//!
//! This method reacts to connect transition events in the disconnected state. The controller
//! connect method is called and the state transits to connected.
//!
//! \param reference to connect event

sc::result Disconnected::react(const EventConnect&)
{
    outermost_context().controller_->connect();
    return transit<Connected>();
}

//! Reacts to disconnect transition events
//!
//! This method reacts to disconnect transition events in the connected state. The controller
//! disconnect method is called and the state transits to disconnected.
//!
//! \param reference to disconnect event

sc::result Connected::react(const EventDisconnect&)
{
    outermost_context().controller_->disconnect();
    return transit<Disconnected>();
}

//! Reacts to arm transition events
//!
//! This method reacts to arm transition events in the connected state. The controller arm method
//! is called and the state transits to armed.
//!
//! \param reference to connect event

sc::result Connected::react(const EventArm&)
{
    outermost_context().controller_->arm();
    return transit<Armed>();
}

//! Reacts to disarm transition events
//!
//! This method reacts to disarm transition events in the armed state. The controller disarm method
//! is called and the state transits to connected.
//!
//! \param reference to connect event

sc::result Armed::react(const EventDisarm&)
{
    outermost_context().controller_->disarm();
    return transit<Connected>();
}

//! Reacts to record start transition events
//!
//! This method reacts to record state transition events in the conarmednected state. The controller
//! start_recording method is called and the state transits to recording.
//!
//! \param reference to connect event

sc::result Armed::react(const EventRecordStart&)
{
    outermost_context().controller_->start_recording();
    return transit<Recording>();
}

//! Reacts to record stop transition events
//!
//! This method reacts to record stop transition events in the recording state. The controller
//! stop_recording state is called and the state transits to armed.
//!
//! \param reference to connect event

sc::result Recording::react(const EventRecordStop&)
{
    outermost_context().controller_->stop_recording();
    return transit<Armed>();
}