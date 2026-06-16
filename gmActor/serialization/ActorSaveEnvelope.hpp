#ifndef GMACTOR_SERIALIZATION_ACTORSAVEENVELOPE_HPP
#define GMACTOR_SERIALIZATION_ACTORSAVEENVELOPE_HPP

/**
 * @file serialization/ActorSaveEnvelope.hpp
 * @brief Versioned save envelope for the full actor store.
 *
 * Wraps `ActorStore` in a versioned JSON envelope compatible with the
 * `gmSave::save_versioned` / `gmSave::load_versioned` API.
 *
 * @par JSON layout
 * @code{.json}
 * {
 *   "_version": 1,
 *   "payload": {
 *     "gmactor_version": "0.1.0",
 *     "store": { ... }
 *   }
 * }
 * @endcode
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/serialization/ActorJson.hpp"
#include "gmSave/gmSave.hpp"

#include <string>

namespace gmActor {

/**
 * @brief Versioned envelope carrying the full actor store state.
 */
struct ActorSaveEnvelope {
    std::string gmactor_version = "0.1.0"; ///< gmActor data schema version
    ActorStore  store;                     ///< The complete actor store
};

// ── ADL serialization ─────────────────────────────────────────────────────────

inline void to_json(nlohmann::json& j, const ActorSaveEnvelope& v)
{
    j = nlohmann::json{
        {"gmactor_version", v.gmactor_version},
        {"store",           v.store}
    };
}

inline void from_json(const nlohmann::json& j, ActorSaveEnvelope& v)
{
    j.at("gmactor_version").get_to(v.gmactor_version);
    j.at("store").get_to(v.store);
}

} // namespace gmActor

#endif // GMACTOR_SERIALIZATION_ACTORSAVEENVELOPE_HPP
