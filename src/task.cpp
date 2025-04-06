#include "task.h"

Task::Task(int taskId, const std::string& desc, int projId) 
    : id(taskId), description(desc), projectId(projId), completed(false) {}

int Task::getId() const { 
    return id; 
}

std::string Task::getDescription() const { 
    return description; 
}

int Task::getProjectId() const { 
    return projectId; 
}

void Task::setDescription(const std::string& desc) { 
    description = desc; 
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