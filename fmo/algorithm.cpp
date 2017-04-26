#include <fmo/algorithm.hpp>
#include <map>

namespace fmo {
    Algorithm::Config::Config()
        : name("median-v1"),
          diff(),
          //
          maxGapX(0.020f),
          minGapY(0.065f),
          maxImageHeight(300),
          minStripHeight(2),
          minStripsInObject(6),
          minStripArea(0.6f),
          minAspect(1.0f),
          matchAspectMax(1.7f),
          matchAreaMax(2.2f),
          matchDistanceMax(10.f),
          matchAngleMax(0.75f),
          matchAspectWeight(1.00f),
          matchAreaWeight(1.35f),
          matchDistanceWeight(0.25f),
          matchAngleWeight(5.00f),
          selectMaxDistance(0.60f),
          //
          minStripsInComponent(2),
          minStripsInCluster(12),
          minClusterLength(2.f),
          heightRatioWeight(1.f),
          distanceWeight(0.f),
          gapsWeight(1.f),
          maxHeightRatioStrips(1.75001f),
          maxHeightRatioInternal(1.75001f),
          maxHeightRatioExternal(1.99999f),
          maxDistance(20.f),
          maxGapsLength(0.75f),
          minMotion(0.25f),
          maxMotion(0.50f),
          pointSetSourceResolution(false) {}

    using AlgorithmRegistry = std::map<std::string, Algorithm::Factory>;

    AlgorithmRegistry& getRegistry() {
        static AlgorithmRegistry registry;
        return registry;
    }

    void registerExplorerV1();
    void registerExplorerV2();
    void registerExplorerV3();
    void registerMedianV1();

    void registerBuiltInFactories() {
        static bool registered = false;
        if (registered) return;
        registered = true;

        registerExplorerV1();
        registerExplorerV2();
        registerExplorerV3();
        registerMedianV1();
    }

    std::unique_ptr<Algorithm> fmo::Algorithm::make(const Config& config, Format format,
                                                    Dims dims) {
        registerBuiltInFactories();
        auto& registry = getRegistry();
        auto it = registry.find(config.name);
        if (it == registry.end()) { throw std::runtime_error("unknown algorithm name"); }
        return it->second(config, format, dims);
    }

    void Algorithm::registerFactory(const std::string& name, const Factory& factory) {
        auto& registry = getRegistry();
        auto it = registry.find(name);
        if (it != registry.end()) { throw std::runtime_error("duplicate algorithm name"); }
        registry.emplace(name, factory);
    }

    std::vector<std::string> Algorithm::listFactories() {
        registerBuiltInFactories();
        const auto& registry = getRegistry();
        std::vector<std::string> result;

        for (auto& entry : registry) { result.push_back(entry.first); }

        return result;
    }
}
