#ifndef jenkinsapi_h
#define jenkinsapi_h

#include <iostream>
#include <vector>
#include <string>

#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

namespace jenkinscpp {
    
    namespace helpers {
        template<typename T>
        T unwrapOptional(const json& j, const T& def) {
            return j.is_null() ? def : j.get<T>();
        }
    }
    
    namespace models {
        
        using namespace helpers;
        
        struct ViewDescription {
            std::string name;
            std::string url;
        };
        
        void from_json(const json& j, ViewDescription& o) {
            o.name = j["name"];
            o.url = j["url"];
        }
        
        struct JobDescription {
            std::string name;
            std::string url;
            std::string color;
        };
        
        void from_json(const json& j, JobDescription& o) {
            o.name = j["name"];
            o.url = j["url"];
            o.color = j["color"];
        }
        
        struct BuildDescription {
            std::string _class;
            uint32_t number;
            std::string url;
        };
        
        void from_json(const json& j, BuildDescription& o) {
            o._class = j["_class"];
            o.number = j["number"];
            o.url = j["url"];
        }
        
        struct Build {
            BuildDescription buildDescription;
            bool building;
            std::string description;
            std::string displayName;
            uint32_t duration;
            uint32_t estimatedDuration;
            std::string fullDisplayName;
            std::string id;
            uint32_t number;
            uint32_t queueId;
            std::string result;
            uint64_t timestamp;
        };
        
        void from_json(const json& j, Build& o) {
            o.buildDescription._class = j["_class"];
            o.buildDescription.number = j["number"];
            o.buildDescription.url = j["url"];

            o.building = j["building"];
            o.description = unwrapOptional(j["description"], std::string(""));
            o.displayName = j["displayName"];
            o.fullDisplayName = j["fullDisplayName"];
            o.id = j["id"];
            
            o.duration = j["duration"];
            o.estimatedDuration = j["estimatedDuration"];
            o.number = j["number"];
            o.queueId = j["queueId"];
            o.result = unwrapOptional(j["result"], std::string(""));
            o.timestamp = j["timestamp"];
        }
        
        struct Job {
            JobDescription jobDescription;
            std::string description;
            std::string displayName, fullDisplayName, fullName;
            bool buildable;
            std::vector<BuildDescription> builds;
            std::shared_ptr<BuildDescription> firstBuild, lastBuild, lastCompletedBuild, lastFailedBuild, lastStableBuild, lastSuccessfulBuild, lastUnstableBuild, lastUnsuccessfulBuild;
            
            uint32_t nextBuildNumber;
        };
        
        void from_json(const json& j, Job& o) {
            o.jobDescription.name = j["name"];
            o.jobDescription.url = j["url"];
            o.jobDescription.color = j["color"];
            
            o.description = j["description"];
            
            o.displayName = j["displayName"];
            o.fullDisplayName = j["fullDisplayName"];
            o.fullName = j["fullName"];
            
            o.buildable = j["buildable"];
            
            o.builds = j["builds"].get<std::vector<BuildDescription>>();
            
            o.firstBuild = j["firstBuild"];
            o.lastBuild = j["lastBuild"];
            o.lastCompletedBuild = j["lastCompletedBuild"];
            o.lastFailedBuild = j["lastFailedBuild"];
            o.lastStableBuild = j["lastStableBuild"];
            o.lastSuccessfulBuild = j["lastSuccessfulBuild"];
            o.lastUnstableBuild = j["lastUnstableBuild"];
            o.lastUnsuccessfulBuild = j["lastUnsuccessfulBuild"];
        }
        
        void from_json(const json& j, std::shared_ptr<BuildDescription>& o) {
            if (!j.is_null()) {
                o = std::make_shared<BuildDescription>(j.get<BuildDescription>());
            }
        }
        
        struct MasterNode {
            std::string mode;
            uint32_t numExecutors;
            
            std::vector<JobDescription> jobs;
            std::vector<ViewDescription> views;
        };
        
        void from_json(const json& j, MasterNode& o) {
            o.mode = j["mode"];
            o.numExecutors = j["numExecutors"];
            
            o.jobs = j["jobs"].get<std::vector<JobDescription>>();
            o.views = j["views"].get<std::vector<ViewDescription>>();
        }
    }
    
    class JenkinsAPI {
    public:
        JenkinsAPI(std::string hostname,
                   int port = 80,
                   std::string basicAuth64 = "") : restClient(hostname.c_str(), port) {
            
            if (!basicAuth64.empty()) {
                headers.emplace("Authorization", "Basic " + basicAuth64);
            }
        };
        
        ~JenkinsAPI() {}
        
        const std::string& getLastError() { return lastError; };
        
        std::shared_ptr<models::MasterNode> getMasterNode() {
            
            return this->get<models::MasterNode>("/api/json");
        };
        
        std::shared_ptr<models::Job> getJob(const std::string& name) {
            
            return this->get<models::Job>("/job/" + name + "/api/json");
        };
        
        std::shared_ptr<models::Build> getBuild(const std::string& jobName, const int id) {
            
            return this->get<models::Build>("/job/" + jobName + "/" + std::to_string(id) + "/api/json");
        };
        
    private:
        httplib::Client restClient;
        httplib::Headers headers;
        
        std::string lastError;
        
        template<typename T>
        std::shared_ptr<T> get(const std::string& apiMethod) {
            
            auto res = restClient.get(apiMethod.c_str(), this->headers);
            if (res && res->status == 200) {
                
                json j = json::parse(res->body);
                return std::make_shared<T>(j);
            } else {
                handleError(res.get());
            }
            
            return nullptr;
        };
        
        void handleError(const httplib::Response* response) {
            
            if (response) {
                
                lastError = "Got response code " + std::to_string(response->status);
            } else {
                lastError = "Unknown api error";
            }
            
            std::cerr << "[ERROR] Jenkins API: " << lastError << std::endl;
        }
    };
}

#endif /* jenkinsapi_h */
