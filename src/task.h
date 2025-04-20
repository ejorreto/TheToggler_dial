#ifndef TASK_H
#define TASK_H

#include <string>

class Task {
private:
    int id;
    std::string description;
    int projectId;

public:
    // Constructor
    Task();
    Task(int taskId, const std::string& desc, int projId);

    // Getters
    int getId() const;
    std::string getDescription() const;
    int getProjectId() const;

    // Setters
    void setDescription(const std::string& desc);
    void setProjectId(int projId);

    // Utility methods
    std::string toString() const;

    // Operator overloading for comparison
    bool operator==(const Task& other) const;
    bool operator<(const Task& other) const;
};

#endif // TASK_H