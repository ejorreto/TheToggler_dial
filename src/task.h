#ifndef TASK_H
#define TASK_H

#include <string>

class Task {
private:
    int id;
    std::string description;
    int projectId;
    std::string projectName = "-";
    std::string project_color;  // Add this line

public:
    // Constructor
    Task();
    Task(int taskId, const std::string& desc, int projId);

    // Getters
    int getId() const;
    std::string getDescription() const;
    int getProjectId() const;
    std::string getProjectName() const;
    std::string getProjectColor() const { return project_color; }

    // Setters
    void setDescription(const std::string& desc);
    void setProjectId(int projId);
    void setProjectName(const std::string& name);
    void setProjectColor(const std::string& color) { project_color = color; }

    // Utility methods
    std::string toString() const;

    // Operator overloading for comparison
    bool operator==(const Task& other) const;
    bool operator<(const Task& other) const;
};

#endif // TASK_H