#include "input.h"

Input::Input() {

}

QString Input::getName()
{
    return "No input";
}

void Input::listen()
{
    listening = true;
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
