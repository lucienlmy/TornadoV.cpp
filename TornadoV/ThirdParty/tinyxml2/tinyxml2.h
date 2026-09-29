#ifndef TINYXML2_INCLUDED
#define TINYXML2_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

namespace tinyxml2 {

class XMLElement {
public:
    const char* Name() const { return name.c_str(); }
    const char* Attribute(const char* attr) const {
        auto it = attributes.find(attr);
        return (it != attributes.end()) ? it->second.c_str() : nullptr;
    }
    XMLElement* FirstChildElement(const char* childName = nullptr) const {
        for (auto child : children) {
            if (!childName || child->name == childName) return child;
        }
        return nullptr;
    }

    XMLElement* InsertNewChild(const char* childName) {
        XMLElement* element = new XMLElement();
        element->name = childName;
        element->parent = this;
        children.push_back(element);
        return element;
    }
    XMLElement* NextSiblingElement(const char* siblingName = nullptr) const {
        if (!parent) return nullptr;
        bool foundSelf = false;
        for (auto sibling : parent->children) {
            if (foundSelf) {
                if (!siblingName || sibling->name == siblingName) return sibling;
            }
            if (sibling == this) foundSelf = true;
        }
        return nullptr;
    }

    void SetText(const char* text) {
        textContent = text ? text : "";
    }

    void DeleteAttribute(const char* attr) {
        attributes.erase(attr);
    }

    std::string name;
    std::map<std::string, std::string> attributes;
    std::vector<XMLElement*> children;
    XMLElement* parent = nullptr;
    std::string textContent;

    ~XMLElement() {
        for (auto child : children) delete child;
    }
};

class XMLDocument {
public:
    XMLElement* RootElement() const { return root; }
    void LoadFile(const char* path) {
        if (root) { delete root; root = nullptr; }
        std::ifstream file(path);
        if (!file.is_open()) return;
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        Parse(content);
    }

    void SaveFile(const char* path) {
        if (!root) return;
        std::ofstream file(path);
        if (!file.is_open()) return;
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        Serialize(file, root, 0);
    }

    ~XMLDocument() { delete root; }

private:
    void Serialize(std::ostream& os, XMLElement* element, int indent) {
        for (int i = 0; i < indent; ++i) os << "  ";
        os << "<" << element->name;
        for (auto const& [key, val] : element->attributes) {
            os << " " << key << "=\"" << val << "\"";
        }
        if (element->children.empty() && element->textContent.empty()) {
            os << " />\n";
        } else {
            os << ">";
            if (!element->textContent.empty()) {
                os << element->textContent;
            }
            if (!element->children.empty()) {
                os << "\n";
                for (auto child : element->children) {
                    Serialize(os, child, indent + 1);
                }
                for (int i = 0; i < indent; ++i) os << "  ";
            }
            os << "</" << element->name << ">\n";
        }
    }

    void Parse(const std::string& xml) {
        size_t pos = 0;
        XMLElement* current = nullptr;
        
        while (pos < xml.size()) {
            pos = xml.find('<', pos);
            if (pos == std::string::npos) break;
            
            if (xml[pos + 1] == '?' || xml[pos + 1] == '!') { // Skip <?xml...?> or <!--...-->
                pos = xml.find('>', pos) + 1;
                continue;
            }

            if (xml[pos + 1] == '/') { // Closing tag
                if (current) current = current->parent;
                pos = xml.find('>', pos) + 1;
                continue;
            }

            // Opening tag
            size_t end = xml.find('>', pos);
            if (end == std::string::npos) break;

            bool selfClosing = (xml[end - 1] == '/');
            std::string tagContent = xml.substr(pos + 1, end - pos - (selfClosing ? 2 : 1));
            
            size_t spacePos = tagContent.find(' ');
            std::string tagName = tagContent.substr(0, spacePos);
            
            XMLElement* element = new XMLElement();
            element->name = tagName;
            element->parent = current;

            // Parse attributes
            if (spacePos != std::string::npos) {
                std::string attrStr = tagContent.substr(spacePos + 1);
                size_t attrPos = 0;
                while (attrPos < attrStr.size()) {
                    size_t eqPos = attrStr.find('=', attrPos);
                    if (eqPos == std::string::npos) break;
                    
                    std::string key = attrStr.substr(attrPos, eqPos - attrPos);
                    while (!key.empty() && isspace(key[0])) key.erase(0, 1);
                    while (!key.empty() && isspace(key.back())) key.pop_back();

                    size_t quoteStart = attrStr.find('"', eqPos);
                    if (quoteStart == std::string::npos) break;
                    size_t quoteEnd = attrStr.find('"', quoteStart + 1);
                    if (quoteEnd == std::string::npos) break;

                    std::string val = attrStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    element->attributes[key] = val;
                    attrPos = quoteEnd + 1;
                }
            }

            if (!root) {
                root = element;
            } else if (current) {
                current->children.push_back(element);
            }

            if (!selfClosing) {
                current = element;
                // Check for text content before closing tag
                size_t nextOpen = xml.find('<', end + 1);
                size_t closeStart = xml.find("</", end + 1);
                if (closeStart != std::string::npos && (nextOpen == std::string::npos || closeStart < nextOpen)) {
                    std::string text = xml.substr(end + 1, closeStart - (end + 1));
                    // Trim whitespace
                    size_t start = text.find_first_not_of(" \t\n\r");
                    size_t endText = text.find_last_not_of(" \t\n\r");
                    if (start != std::string::npos) {
                        element->textContent = text.substr(start, endText - start + 1);
                    }
                }
            }
            pos = end + 1;
        }
    }
    XMLElement* root = nullptr;
};

} // namespace tinyxml2

#endif
