#include "task.h"

Task::Task(int taskId, const std::string& desc) 
    : id(taskId), description(desc), completed(false) {}

int Task::getId() const { 
    return id; 
}

std::string Task::getDescription() const { 
    return description; 
}

bool Task::isCompleted() const { 
    return completed; 
}

void Task::setDescription(const std::string& desc) { 
    description = desc; 
}

void Task::setCompleted(bool status) { 
    completed = status; 
}

std::string Task::toString() const {
    return "Task " + std::to_string(id) + ": " + description + 
           (completed ? " (Completed)" : " (Pending)");
}

bool Task::operator==(const Task& other) const {
    return id == other.id;
}

bool Task::operator<(const Task& other) const {
    return id < other.id;
}