# JenkinsCpp
Simple C++ client for interacting with the Jenkins API

# Dependencies
https://github.com/nlohmann/json
https://github.com/yhirose/cpp-httplib

# Example
```c++
#include "jenkinsapi.hpp"

jenkinscpp::JenkinsAPI api("localhost", 8080, "dGVzdDp0ZXN0");

auto jenkins = api.GetMasterNode();
if (jenkins) {
    for (const auto& jobDesc : jenkins->jobs) {

        auto job = api.GetJob(jobDesc.name);
        if (job && job->lastBuild)  {

            std::cout << "Found Job: " << jobDesc.name << " # " << job->description << std::endl;

            for (const auto& buildDesc : job->builds) {

                auto build = api.GetBuild(job->jobDescription.name, buildDesc.number);
                if (build) {

                    std::cout << "\t" << "Build: " << build->displayName << " # " << build->result << std::endl;
                }
            }
        }
    }
}
```
