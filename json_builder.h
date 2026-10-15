#pragma once

#include "json.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <optional> // optional

namespace json {

class Builder {
public:
    class KeyValueContext;
    class DictItemContext;
    class ArrayItemContext;

    Builder();
 // помеченные "nest" вложены в класс
    DictItemContext StartDict(); //nest
    ArrayItemContext StartArray(); //nest
    Builder& Value(Node value);
    Node Build();
    KeyValueContext Key(std::string key); //nest
    Builder& EndDict();
    Builder& EndArray();

private:
    void OnValue(Node value);
    void OnKey(std::string key);
    void OnStartDict();
    void OnEndDict();
    void OnStartArray();
    void OnEndArray();
    void StartContainer(Node container);

    Node root_;
    std::vector<Node*> nodes_stack_;
    std::optional<std::string> key_; // 1 поле
    bool is_root_set_ = false;
};

class Builder::KeyValueContext {
public:
    explicit KeyValueContext(Builder& builder);
    DictItemContext Value(Node value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
private:
    Builder& builder_;
};

class Builder::DictItemContext {
public:
    explicit DictItemContext(Builder& builder);
    KeyValueContext Key(std::string key);
    Builder& EndDict();
private:
    Builder& builder_;
};

class Builder::ArrayItemContext {
public:
    explicit ArrayItemContext(Builder& builder);
    ArrayItemContext& Value(Node value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndArray();
private:
    Builder& builder_;
};

} // namespace json