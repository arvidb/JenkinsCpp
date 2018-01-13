#ifndef jenkinsapi_h
#define jenkinsapi_h

#include <iostream>
#include <vector>
#include <string>

#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

namespace jenkinscpp {
    
    namespace models {
        
        struct View {
            std::string name;
            std::string url;
        };
        
        void from_json(const json& j, View& o) {
            o.name = j.at("name").get<std::string>();
            o.url = j.at("url").get<std::string>();
        }
        
        struct JobDescription {
            std::string name;
            std::string url;
            std::string color;
        };
        
        void from_json(const json& j, JobDescription& o) {
            o.name = j["name"].get<std::string>();
            o.url = j["url"].get<std::string>();
            o.color = j["color"].get<std::string>();
        }
        
        struct BuildDescription {
            std::string _class;
            uint32_t number;
            std::string url;
        };
        
        void from_json(const json& j, BuildDescription& o) {
            o._class = j["_class"].get<std::string>();
            o.number = j["number"].get<int>();
            o.url = j["url"].get<std::string>();
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
            o.description = j["description"].is_string() ? j["description"] : "";
            o.displayName = j["displayName"];
            o.fullDisplayName = j["fullDisplayName"];
            o.id = j["id"];
            
            o.duration = j["duration"];
            o.estimatedDuration = j["estimatedDuration"];
            o.number = j["number"];
            o.queueId = j["queueId"];
            o.result = j["result"];
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
            o.jobDescription.name = j["name"].get<std::string>();
            o.jobDescription.url = j["url"].get<std::string>();
            o.jobDescription.color = j["color"].get<std::string>();
            
            o.description = j["description"].get<std::string>();
            
            o.displayName = j["displayName"].get<std::string>();
            o.fullDisplayName = j["fullDisplayName"].get<std::string>();
            o.fullName = j["fullName"].get<std::string>();
            
            o.buildable = j["buildable"].get<bool>();
            
            o.builds = j["builds"].get<std::vector<BuildDescription>>();
            
            o.firstBuild = j["firstBuild"].get<std::shared_ptr<BuildDescription>>();
            o.lastBuild = j["lastBuild"].get<std::shared_ptr<BuildDescription>>();
            o.lastCompletedBuild = j["lastCompletedBuild"].get<std::shared_ptr<BuildDescription>>();
            o.lastFailedBuild = j["lastFailedBuild"].get<std::shared_ptr<BuildDescription>>();
            o.lastStableBuild = j["lastStableBuild"].get<std::shared_ptr<BuildDescription>>();
            o.lastSuccessfulBuild = j["lastSuccessfulBuild"].get<std::shared_ptr<BuildDescription>>();
            o.lastUnstableBuild = j["lastUnstableBuild"].get<std::shared_ptr<BuildDescription>>();
            o.lastUnsuccessfulBuild = j["lastUnsuccessfulBuild"].get<std::shared_ptr<BuildDescription>>();
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
            std::vector<View> views;
        };
        
        void from_json(const json& j, MasterNode& o) {
            o.mode = j["mode"].get<std::string>();
            o.numExecutors = j["numExecutors"].get<int>();
            
            o.jobs = j["jobs"].get<std::vector<JobDescription>>();
            o.views = j["views"].get<std::vector<View>>();
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
        
        const std::string& GetLastError() { return lastError; };
        
        std::shared_ptr<models::MasterNode> GetMasterNode() {
            
            return this->Get<models::MasterNode>("/api/json");
        };
        
        std::shared_ptr<models::Job> GetJob(const std::string& name) {
            
            return this->Get<models::Job>("/job/" + name + "/api/json");
        };
        
        std::shared_ptr<models::Build> GetBuild(const std::string& jobName, const int id) {
            
            return this->Get<models::Build>("/job/" + jobName + "/" + std::to_string(id) + "/api/json");
        };
        
    private:
        httplib::Client restClient;
        httplib::Headers headers;
        
        std::string lastError;
        
        template<typename T>
        std::shared_ptr<T> Get(const std::string& apiMethod) {
            
            auto res = restClient.get(apiMethod.c_str(), this->headers);
            if (res && res->status == 200) {
                
                json j = json::parse(res->body);
                return std::make_shared<T>(j);
            } else {
                ErrorHandler(res.get());
            }
            
            return nullptr;
        };
        
        void ErrorHandler(const httplib::Response* response) {
            
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
