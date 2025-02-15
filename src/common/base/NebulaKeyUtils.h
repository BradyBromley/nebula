/* Copyright (c) 2018 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef COMMON_BASE_NEBULAKEYUTILS_H_
#define COMMON_BASE_NEBULAKEYUTILS_H_

#include "base/Base.h"

namespace nebula {

/**
 * VertexKeyUtils:
 * type(1) + partId(3) + vertexId(8) + tagId(4) + version(8)
 *
 * EdgeKeyUtils:
 * type(1) + partId(3) + srcId(8) + edgeType(4) + edgeRank(8) + dstId(8) + version(8)
 *
 * */

enum NebulaKeyType : uint32_t {
    kData              = 0x00000001,
    kIndex             = 0x00000002,
    kUUID              = 0x00000003,
    kSystem            = 0x00000004,
};

enum NebulaSystemKeyType : uint32_t {
    kSystemCommit      = 0x00000001,
    kSystemPart        = 0x00000002,
};

/**
 * This class supply some utils for transition between Vertex/Edge and key in kvstore.
 * */
class NebulaKeyUtils final {
public:
    ~NebulaKeyUtils() = default;
    /**
     * Generate vertex key for kv store
     * */
    static std::string vertexKey(PartitionID partId, VertexID vId,
                                 TagID tagId, TagVersion tv);

    /**
     * Generate edge key for kv store
     * */
    static std::string edgeKey(PartitionID partId, VertexID srcId,
                               EdgeType type, EdgeRanking rank,
                               VertexID dstId, EdgeVersion ev);

    static std::string systemCommitKey(PartitionID partId);

    static std::string systemPartKey(PartitionID partId);

    /**
     * Prefix for
     * */
    static std::string vertexPrefix(PartitionID partId, VertexID vId, TagID tagId);

    /**
     * Prefix for srcId edges with some edgeType
     * */
    static std::string edgePrefix(PartitionID partId, VertexID srcId, EdgeType type);

    static std::string vertexPrefix(PartitionID partId, VertexID vId);

    static std::string edgePrefix(PartitionID partId, VertexID vId);

    static std::string systemPrefix();

    static std::string prefix(PartitionID partId, VertexID src, EdgeType type,
                              EdgeRanking ranking, VertexID dst);

    static std::string prefix(PartitionID partId);

    static bool isVertex(const folly::StringPiece& rawKey) {
        constexpr uint32_t tagMask  = 0x40000000;
        constexpr uint32_t typeMask = 0x000000FF;
        constexpr int32_t len = static_cast<int32_t>(sizeof(NebulaKeyType));
        if (NebulaKeyType::kData != (readInt<int32_t>(rawKey.data(), len) & typeMask)) {
            return false;
        }
        auto offset = sizeof(PartitionID) + sizeof(VertexID);
        TagID tagId =  readInt<TagID>(rawKey.data() + offset, sizeof(TagID));
        return !(tagId & tagMask);
    }

    static TagID getTagId(const folly::StringPiece& rawKey) {
        CHECK_EQ(rawKey.size(), kVertexLen);
        auto offset = sizeof(PartitionID) + sizeof(VertexID);
        return readInt<TagID>(rawKey.data() + offset, sizeof(TagID));
    }

    static bool isEdge(const folly::StringPiece& rawKey) {
        constexpr uint32_t edgeMask = 0x40000000;
        constexpr uint32_t typeMask = 0x000000FF;
        constexpr int32_t len = static_cast<int32_t>(sizeof(NebulaKeyType));
        if (NebulaKeyType::kData != (readInt<int32_t>(rawKey.data(), len) & typeMask)) {
            return false;
        }
        auto offset = sizeof(PartitionID) + sizeof(VertexID);
        EdgeType type = readInt<EdgeType>(rawKey.data() + offset, sizeof(EdgeType));
        return type & edgeMask;
    }

    static bool isSystemCommit(const folly::StringPiece& rawKey) {
        if (rawKey.size() != kSystemLen) {
            return false;
        }
        auto position = rawKey.data() + sizeof(PartitionID);
        auto len = sizeof(NebulaSystemKeyType);
        return NebulaSystemKeyType::kSystemCommit == readInt<uint32_t>(position, len);
    }

    static bool isSystemPart(const folly::StringPiece& rawKey) {
        if (rawKey.size() != kSystemLen) {
            return false;
        }
        auto position = rawKey.data() + sizeof(PartitionID);
        auto len = sizeof(NebulaSystemKeyType);
        return NebulaSystemKeyType::kSystemPart == readInt<uint32_t>(position, len);
    }

    static VertexID getSrcId(const folly::StringPiece& rawKey) {
        CHECK_EQ(rawKey.size(), kEdgeLen);
        return readInt<VertexID>(rawKey.data() + sizeof(PartitionID), sizeof(VertexID));
    }

    static VertexID getDstId(const folly::StringPiece& rawKey) {
        CHECK_EQ(rawKey.size(), kEdgeLen);
        auto offset = sizeof(PartitionID) + sizeof(VertexID) +
                      sizeof(EdgeType) + sizeof(EdgeRanking);
        return readInt<VertexID>(rawKey.data() + offset, sizeof(VertexID));
    }

    static EdgeType getEdgeType(const folly::StringPiece& rawKey) {
        CHECK_EQ(rawKey.size(), kEdgeLen);
        constexpr int32_t edgeMask = 0xBFFFFFFF;
        auto offset = sizeof(PartitionID) + sizeof(VertexID);
        EdgeType type = readInt<EdgeType>(rawKey.data() + offset, sizeof(EdgeType));
        return type > 0 ? type & edgeMask : type;
    }

    static EdgeRanking getRank(const folly::StringPiece& rawKey) {
        CHECK_EQ(rawKey.size(), kEdgeLen);
        auto offset = sizeof(PartitionID) + sizeof(VertexID) + sizeof(EdgeType);
        return readInt<EdgeRanking>(rawKey.data() + offset, sizeof(EdgeRanking));
    }

    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value, T>::type
    readInt(const char* data, int32_t len) {
        CHECK_GE(len, sizeof(T));
        return *reinterpret_cast<const T*>(data);
    }

    static bool isDataKey(const folly::StringPiece& key) {
        constexpr uint32_t typeMask = 0x000000FF;
        constexpr int32_t len = static_cast<int32_t>(sizeof(NebulaKeyType));
        return NebulaKeyType::kData == (readInt<int32_t>(key.data(), len) & typeMask);
    }

    static bool isIndexKey(const folly::StringPiece& key) {
        constexpr uint32_t typeMask = 0x000000FF;
        constexpr int32_t len = static_cast<int32_t>(sizeof(NebulaKeyType));
        return NebulaKeyType::kIndex == (readInt<int32_t>(key.data(), len) & typeMask);
    }

    static folly::StringPiece keyWithNoVersion(const folly::StringPiece& rawKey) {
        // TODO(heng) We should change the method if varint data version supportted.
        return rawKey.subpiece(0, rawKey.size() - sizeof(int64_t));
    }

private:
    NebulaKeyUtils() = delete;

private:
    static constexpr int32_t kVertexLen = sizeof(PartitionID) + sizeof(VertexID)
                                        + sizeof(TagID) + sizeof(TagVersion);

    static constexpr int32_t kEdgeLen = sizeof(PartitionID) + sizeof(VertexID)
                                      + sizeof(EdgeType) + sizeof(VertexID)
                                      + sizeof(EdgeRanking) + sizeof(EdgeVersion);

    static constexpr int32_t kSystemLen = sizeof(PartitionID) + sizeof(NebulaSystemKeyType);
};

}  // namespace nebula
#endif  // COMMON_BASE_NEBULAKEYUTILS_H_

