#pragma once

namespace tmt {

/* Ray definition. */
struct Ray {
    glm::vec3 origin {};
    glm::vec3 direction {};
};

/* Ray hit data. */
struct Hit {
    float distance = 0.0f;
};

}  // namespace tmt
