#include "preprocessor.hpp"
#include <unordered_map>
#include <string>
#include <cctype>
#include <stdexcept>

std::string Preprocessor::process(const std::string &source) {
    std::unordered_map<std::string,std::string> masks;
    int runningSum = 0;
    std::string output;
    size_t i = 0, n = source.size();

    while (i < n) {
        char c = source[i];

        if (c == '/' && i+1 < n && source[i+1] == '/') {
            i += 2;
            while (i < n && source[i] != '\n') ++i;
            if (i < n && source[i] == '\n') {
                output.push_back('\n');
                ++i;
            }
            continue;
        }

        if (n-i >= 4 && source.compare(i,4,"mask")==0 && (i+4==n || !std::isalnum(source[i+4]))) {
            size_t maskStart = i;
            size_t maskLineStart = output.size();

            i +=4; while (i<n && std::isspace(source[i])) ++i;
            size_t nameStart = i;
            while (i<n && (std::isalnum(source[i]) || source[i]=='_')) ++i;
            std::string maskName = source.substr(nameStart,i-nameStart);
            while (i<n && source[i]!='{') ++i; if (i<n && source[i]=='{') ++i;

            while (i<n) {
                while (i<n && std::isspace(source[i])) {
                    if (source[i] == '\n') output.push_back('\n'); // keep newlines
                    ++i;
                }

                if (i+1<n && source[i]=='/' && source[i+1]=='/') {
                    i += 2;
                    while (i<n && source[i] != '\n') ++i;
                    if (i<n && source[i]=='\n') { output.push_back('\n'); ++i; }
                    continue;
                }

                if (i>=n || source[i]=='}') { if(i<n) ++i; break; }

                size_t fstart = i;
                while (i<n && (std::isalnum(source[i]) || source[i]=='_')) ++i;
                std::string field = source.substr(fstart,i-fstart);
                while (i<n && std::isspace(source[i])) {
                    if (source[i]=='\n') output.push_back('\n');
                    ++i;
                }
                if (i<n && source[i]==':') ++i;
                while (i<n && std::isspace(source[i])) {
                    if (source[i]=='\n') output.push_back('\n');
                    ++i;
                }
                size_t numStart = i;
                while (i<n && std::isdigit(source[i])) ++i;
                std::string number = source.substr(numStart,i-numStart);
                int val = number.empty()?0:std::stoi(number);

                if (field != "any") {
                    int start = runningSum;
                    int end   = start + val;
                    if (!maskName.empty()) {
                        if (val == 1)
                            masks[maskName + "." + field] = std::to_string(start);
                        else
                            masks[maskName + "." + field] =
                                std::to_string(start) + ":" + std::to_string(end);
                    }
                    runningSum = end;
                } else {
                    runningSum += val;
                }

                while(i<n && source[i]!=';' && source[i]!='}') {
                    if (source[i]=='\n') output.push_back('\n');
                    ++i;
                }
                if(i<n && source[i]==';') ++i;
            }

            if (i<n && (source[i]==';' || source[i]=='\n')) {
                if (source[i]=='\n') output.push_back('\n');
                ++i;
            }
            runningSum = 0;

            continue;
        }

        output.push_back(source[i++]);
    }

	bool changed = true;
	while (changed) {
	    changed = false;
	    for (auto &kv : masks) {
	        const std::string &key = kv.first;
	        const std::string &val = kv.second;
	        size_t pos = 0;
	        while ((pos = output.find(key, pos)) != std::string::npos) {
	            bool okBefore = (pos == 0) || !(std::isalnum(output[pos - 1]) || output[pos - 1] == '_');
	            size_t afterPos = pos + key.size();
	            bool okAfter = (afterPos >= output.size()) || !(std::isalnum(output[afterPos]) || output[afterPos] == '_');
	            if (okBefore && okAfter) {
	                output.replace(pos, key.size(), val);
	                pos += val.size();
	                changed = true;
	            } else {
	                pos += key.size();
	            }
	        }
	    }
	}

    size_t dotPos=0;
    while((dotPos = output.find('.',dotPos)) != std::string::npos) {
        size_t start = dotPos;
        while(start>0 && std::isalnum(output[start-1])) --start;
        size_t end = dotPos+1;
        while(end<output.size() && std::isalnum(output[end])) ++end;
        std::string key = output.substr(start,end-start);
        if(masks.find(key)==masks.end())
            throw std::runtime_error("Unknown mask field "+key);
        dotPos = end;
    }

    return output;
}
