#pragma once

struct NodeIndex {
    int val = -1;

    bool isEmpty() const {
        return val == -1;
    }

    static NodeIndex rootIndex() {
        return NodeIndex{0};
    }
};

