#include "json_builder.h"
#include <stdexcept>

using namespace std::literals;

namespace json {

Builder::Builder() = default;

void Builder::OnValue(Node value) {
    if (is_root_set_ && nodes_stack_.empty()) throw std::logic_error("Value after build"s);
    if (!nodes_stack_.empty() && !nodes_stack_.back()->IsArray() && !key_.has_value()) {
        throw std::logic_error("Value not in array or after key"s);
    }

    if (nodes_stack_.empty()) {
        root_ = std::move(value);
    } else if (nodes_stack_.back()->IsArray()) {
        nodes_stack_.back()->AsArray().emplace_back(std::move(value));
    } else {
        nodes_stack_.back()->AsDict()[std::move(*key_)] = std::move(value);
        key_.reset();
    }
    is_root_set_ = true;
}

// move key
void Builder::OnKey(std::string key) {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict() || key_.has_value()) {
        throw std::logic_error("Invalid Key call"s);
    }
    key_ = std::move(key);
}

// container
void Builder::StartContainer(Node container) {
    if (is_root_set_ && nodes_stack_.empty())
        throw std::logic_error("StartContainer after build"s);
    if (!nodes_stack_.empty() && !nodes_stack_.back()->IsArray() && !key_.has_value())
        throw std::logic_error("StartContainer not in array or after key"s);

    Node* new_node = nullptr;
    if (nodes_stack_.empty()) {
        root_ = std::move(container);
        new_node = &root_;
    } else if (nodes_stack_.back()->IsArray()) {
        nodes_stack_.back()->AsArray().emplace_back(std::move(container));
        new_node = &nodes_stack_.back()->AsArray().back();
    } else {
        new_node = &(nodes_stack_.back()->AsDict()[std::move(*key_)] = std::move(container));
        key_.reset();
    }
    nodes_stack_.push_back(new_node);
    is_root_set_ = true;
}

void Builder::OnStartDict() {
    StartContainer(Dict{});
}

void Builder::OnStartArray() {
    StartContainer(Array{});
}

void Builder::OnEndDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict() || key_.has_value()) {
        throw std::logic_error("Invalid EndDict call"s);
    }
    nodes_stack_.pop_back();
}

void Builder::OnEndArray() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("Invalid EndArray call"s);
    }
    nodes_stack_.pop_back();
}

Node Builder::Build() {
    if (!is_root_set_ || !nodes_stack_.empty()) {
        throw std::logic_error("JSON object is not finalized"s);
    }
    return root_;
}

Builder::DictItemContext Builder::StartDict() { OnStartDict(); return DictItemContext(*this); }
Builder::ArrayItemContext Builder::StartArray() { OnStartArray(); return ArrayItemContext(*this); }
Builder& Builder::Value(Node value) { OnValue(std::move(value)); return *this; }
Builder::KeyValueContext Builder::Key(std::string key) { OnKey(std::move(key)); return KeyValueContext(*this); }
Builder& Builder::EndDict() { OnEndDict(); return *this; }
Builder& Builder::EndArray() { OnEndArray(); return *this; }

Builder::KeyValueContext::KeyValueContext(Builder& builder) : builder_(builder) {}
Builder::DictItemContext Builder::KeyValueContext::Value(Node value) { builder_.OnValue(std::move(value)); return Builder::DictItemContext(builder_); }
Builder::DictItemContext Builder::KeyValueContext::StartDict() { builder_.OnStartDict(); return Builder::DictItemContext(builder_); }
Builder::ArrayItemContext Builder::KeyValueContext::StartArray() { builder_.OnStartArray(); return Builder::ArrayItemContext(builder_); }

Builder::DictItemContext::DictItemContext(Builder& builder) : builder_(builder) {}
Builder::KeyValueContext Builder::DictItemContext::Key(std::string key) { builder_.OnKey(std::move(key)); return Builder::KeyValueContext(builder_); }
Builder& Builder::DictItemContext::EndDict() { builder_.OnEndDict(); return builder_; }

Builder::ArrayItemContext::ArrayItemContext(Builder& builder) : builder_(builder) {}
Builder::ArrayItemContext& Builder::ArrayItemContext::Value(Node value) { builder_.OnValue(std::move(value)); return *this; }
Builder::DictItemContext Builder::ArrayItemContext::StartDict() { builder_.OnStartDict(); return Builder::DictItemContext(builder_); }
Builder::ArrayItemContext Builder::ArrayItemContext::StartArray() { builder_.OnStartArray(); return Builder::ArrayItemContext(builder_); }
Builder& Builder::ArrayItemContext::EndArray() { builder_.OnEndArray(); return builder_; }

} // namespace json