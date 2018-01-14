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
        
        enum class LogLevel {
            Error = 0, Info
        };
        
        template <LogLevel L>
        void Log(const std::string& func, const std::string& msg) {
            std::string log = "JenkinsCpp - [" + func + "] " + msg;
            switch (L) {
                case LogLevel::Error:
                    std::cerr << "[ERROR] " << log << std::endl;
                    break;
                default:
                    std::cout << "[INFO] " << log << std::endl;
                    break;
            }
        }
        
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
    
    using logLevel = helpers::LogLevel;
    
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
            
            return this->getJSON<models::MasterNode>("/api/json");
        };
        
        std::shared_ptr<models::Job> getJob(const std::string& name) {
            
            return this->getJSON<models::Job>("/job/" + name + "/api/json");
        };
        
        void obtainCrumb() {
            
            auto res = this->getJSON<json>("/crumbIssuer/api/json");
            if (res) {
                this->headers.emplace(res->at("crumbRequestField"), res->at("crumb"));
            } else if (lastErrorCode == 404) {
                helpers::Log<logLevel::Info>(__func__, "Jenkins does not require a crumb");
            }
        }
        
        void buildJob(const std::string& name) {
            
            this->post("/job/" + name + "/build", "", "");
        }
        
        std::shared_ptr<models::Build> getBuild(const std::string& jobName, const int id) {
            
            return this->getJSON<models::Build>("/job/" + jobName + "/" + std::to_string(id) + "/api/json");
        };
        
    private:
        httplib::Client restClient;
        httplib::Headers headers;
        
        std::string lastError;
        int lastErrorCode;
        
        template<typename T>
        std::shared_ptr<T> getJSON(const std::string& apiMethod) {
            
            resetLastError();
            
            auto res = restClient.get(apiMethod.c_str(), this->headers);
            if (res && res->status == 200) {
                
                json j = json::parse(res->body);
                return std::make_shared<T>(j);
            } else {
                handleError("GET", res.get());
            }
            
            return nullptr;
        };
        
        void post(const std::string& apiMethod, const std::string& body, const std::string& type) {
            
            resetLastError();
            
            auto res = restClient.post(apiMethod.c_str(), this->headers, body, type.c_str());
            if (res && res->status == 201) {
                
                // Request success
            } else {
                
                if (res && res->status == 403) {
                    if (res->body.find("No valid crumb") != std::string::npos) {
                        handleError("POST", res.get(), "No valid crumb in request, obtain crumb headers by calling obtainCrumb()");
                    }
                } else {
                    handleError("POST", res.get());
                }
            }
        };
        
        void resetLastError() {
            this->lastError = "";
            this->lastErrorCode = -1;
        }
        
        void handleError(const std::string& method, const httplib::Response* response, const std::string& reason = "") {
            
            if (!reason.empty()) {
                
                lastError = reason;
            } else if (response) {
                
                lastError = "Got response code " + std::to_string(response->status);
            } else {
                
                lastError = "Unknown api error";
            }
            
            if (response) {
                lastErrorCode = response->status;
            }
            
            helpers::Log<logLevel::Error>(method, lastError);
        }
    };
}

#endif /* jenkinsapi_h */
