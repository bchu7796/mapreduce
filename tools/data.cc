#include "data.h"

result_pair::result_pair() {
    this->id = -1;
    this->result = -1;
}

result_pair::result_pair(int32_t id, int32_t result) {
    this->id = id;
    this->result = result;
}

bool result_pair::operator==(const result_pair& a) {
    return (this->id == a.id) && (this->result == a.result);
}

bool result_pair::operator!=(const result_pair& a) {
    return (this->id != a.id) || (this->result != a.result);
}