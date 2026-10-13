#pragma once

// Running keeps a node active. Success and Failure finish its current execution.
enum class BT_NodeResult
{
    Running,
    Success,
    Failure
};
