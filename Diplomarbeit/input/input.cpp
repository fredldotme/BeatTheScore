#include "input.h"

Input::Input() {

}

QString Input::getName()
{
    return "No input";
}

bool Input::listen()
{
    listening = true;
    return true;
}

void Input::stop()
{
    listening = false;
}

bool Input::isListening()
{
    return listening;
}

InputType Input::getType()
{
    return DEMO;
}
