#include "task.h"

Task::Task() {}

Task::Task(int taskId, const std::string &desc, int projId)
    : id(taskId), description(desc), projectId(projId) {}

int Task::getId() const
{
  return id;
}

std::string Task::getDescription() const
{
  return description;
}

int Task::getProjectId() const
{
  return projectId;
}

std::string Task::getProjectName() const
{
  return projectName;
}

void Task::setDescription(const std::string &desc)
{
  description = desc;
}

void Task::setProjectId(int projId)
{
  projectId = projId;
}

void Task::setProjectName(const std::string &name)
{
  projectName = name;
}

std::string Task::toString() const
{
  return "Task " + std::to_string(id) + ": " + description;
}

bool Task::operator==(const Task &other) const
{
  return id == other.id;
}

bool Task::operator<(const Task &other) const
{
  return id < other.id;
}