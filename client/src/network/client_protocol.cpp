#include "clearspace/client/network/client_protocol.hpp"

#include <nlohmann/json.hpp>

namespace clearspace::client {
    namespace {

        using json = nlohmann::json;

        clearspace::transport::Expected<QString, std::string> getRequiredString(const json& object, const char* key) {
            if (!object.contains(key)) {
                return std::string("missing field: ") + key;
            }
            if (!object.at(key).is_string()) {
                return std::string("invalid field type: ") + key;
            }
            return QString::fromStdString(object.at(key).get<std::string>());
        }

        template <class T>
        clearspace::transport::Expected<T, QString> getRequiredNumber(const json& object, const char* key) {
            if (!object.contains(key)) {
                return QString("missing field: %1").arg(key);
            }
            try {
                return object.at(key).get<T>();
            } catch (const json::exception&) {
                return QString("invalid field type: %1").arg(key);
            }
        }

        clearspace::transport::Expected<QPointF, QString> getRequiredPoint(const json& object, const char* key) {
            if (!object.contains(key)) {
                return QString("missing field: %1").arg(key);
            }
            if (!object.at(key).is_object()) {
                return QString("invalid field type: %1").arg(key);
            }
            const auto& pointObj = object.at(key);
            auto x = getRequiredNumber<double>(pointObj, "x");
            if (!x) {
                return x.error();
            }
            auto y = getRequiredNumber<double>(pointObj, "y");
            if (!y) {
                return y.error();
            }
            return QPointF(x.value(), y.value());
        }

        json makePointJson(const QPointF& point) {
            return json{{"x", point.x()}, {"y", point.y()}};
        }

        clearspace::transport::Expected<StrokeData, QString> parseStroke(const json& object) {
            StrokeData stroke;

            auto id = getRequiredNumber<StrokeId>(object, "id");
            if (!id) {
                return id.error();
            }
            auto author = getRequiredNumber<ClientId>(object, "author");
            if (!author) {
                return author.error();
            }
            auto color = getRequiredNumber<unsigned int>(object, "color");
            if (!color) {
                return color.error();
            }
            auto thickness = getRequiredNumber<unsigned int>(object, "thickness");
            if (!thickness) {
                return thickness.error();
            }
            auto strokeOrder = getRequiredNumber<Seq>(object, "stroke_order");
            if (!strokeOrder) {
                return strokeOrder.error();
            }
            if (!object.contains("points") || !object.at("points").is_array()) {
                return QString("invalid field type: points");
            }
            if (!object.contains("finished") || !object.at("finished").is_boolean()) {
                return QString("invalid field type: finished");
            }

            stroke.id = id.value();
            stroke.author = author.value();
            stroke.color = static_cast<std::uint8_t>(color.value());
            stroke.thickness = static_cast<std::uint8_t>(thickness.value());
            stroke.strokeOrder = strokeOrder.value();
            stroke.finished = object.at("finished").get<bool>();

            for (const auto& pointObj : object.at("points")) {
                if (!pointObj.is_object()) {
                    return QString("invalid field type: points");
                }
                auto x = getRequiredNumber<double>(pointObj, "x");
                if (!x) {
                    return x.error();
                }
                auto y = getRequiredNumber<double>(pointObj, "y");
                if (!y) {
                    return y.error();
                }
                stroke.points.emplace_back(x.value(), y.value());
            }

            return stroke;
        }

        clearspace::transport::Expected<CursorData, QString> parseCursor(const json& object) {
            CursorData cursor;
            auto client = getRequiredNumber<ClientId>(object, "client");
            if (!client) {
                return client.error();
            }
            auto point = getRequiredPoint(object, "point");
            if (!point) {
                return point.error();
            }
            cursor.client = client.value();
            cursor.point = point.value();
            return cursor;
        }

        std::string dumpWithNewline(const json& object) {
            return object.dump() + "\n";
        }

    }

    clearspace::transport::Expected<ServerMessage, QString> parseServerMessage(std::string_view line) {
        json object;
        try {
            object = json::parse(line.begin(), line.end());
        } catch (const json::exception& ex) {
            return QString::fromUtf8(ex.what());
        }

        if (!object.is_object()) {
            return QString("root json value must be an object");
        }

        auto type = getRequiredString(object, "type");
        if (!type) {
            return QString::fromStdString(type.error());
        }

        if (type.value() == "pong") {
            return ServerMessage{std::monostate{}};
        }
        if (type.value() == "room_created") {
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            return ServerMessage{RoomCreatedData{.room = room.value()}};
        }
        if (type.value() == "ok") {
            auto request = getRequiredString(object, "request");
            if (!request) {
                return QString::fromStdString(request.error());
            }
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            return ServerMessage{OkData{.request = request.value(), .room = room.value()}};
        }
        if (type.value() == "error") {
            auto code = getRequiredString(object, "code");
            if (!code) {
                return QString::fromStdString(code.error());
            }
            auto message = getRequiredString(object, "message");
            if (!message) {
                return QString::fromStdString(message.error());
            }
            return ServerMessage{ErrorData{.code = code.value(), .message = message.value()}};
        }
        if (type.value() == "room_snapshot") {
            SnapshotData snapshot;

            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            auto lastSeq = getRequiredNumber<Seq>(object, "last_seq");
            if (!lastSeq) {
                return lastSeq.error();
            }
            if (!object.contains("members") || !object.at("members").is_array()) {
                return QString("invalid field type: members");
            }
            if (!object.contains("strokes") || !object.at("strokes").is_array()) {
                return QString("invalid field type: strokes");
            }
            if (!object.contains("cursors") || !object.at("cursors").is_array()) {
                return QString("invalid field type: cursors");
            }

            snapshot.room = room.value();
            snapshot.lastSeq = lastSeq.value();

            for (const auto& member : object.at("members")) {
                try {
                    snapshot.members.push_back(member.get<ClientId>());
                } catch (const json::exception&) {
                    return QString("invalid field type: members");
                }
            }

            for (const auto& strokeObj : object.at("strokes")) {
                auto stroke = parseStroke(strokeObj);
                if (!stroke) {
                    return stroke.error();
                }
                snapshot.strokes.push_back(std::move(stroke.value()));
            }

            for (const auto& cursorObj : object.at("cursors")) {
                auto cursor = parseCursor(cursorObj);
                if (!cursor) {
                    return cursor.error();
                }
                snapshot.cursors.push_back(std::move(cursor.value()));
            }

            return ServerMessage{std::move(snapshot)};
        }
        if (type.value() == "stroke_begin") {
            StrokeBeginData ev;
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            auto seq = getRequiredNumber<Seq>(object, "seq");
            if (!seq) {
                return seq.error();
            }
            auto author = getRequiredNumber<ClientId>(object, "author");
            if (!author) {
                return author.error();
            }
            auto id = getRequiredNumber<StrokeId>(object, "id");
            if (!id) {
                return id.error();
            }
            auto strokeOrder = getRequiredNumber<Seq>(object, "stroke_order");
            if (!strokeOrder) {
                return strokeOrder.error();
            }
            auto color = getRequiredNumber<unsigned int>(object, "color");
            if (!color) {
                return color.error();
            }
            auto thickness = getRequiredNumber<unsigned int>(object, "thickness");
            if (!thickness) {
                return thickness.error();
            }
            auto start = getRequiredPoint(object, "start");
            if (!start) {
                return start.error();
            }
            ev.room = room.value();
            ev.seq = seq.value();
            ev.author = author.value();
            ev.id = id.value();
            ev.strokeOrder = strokeOrder.value();
            ev.color = static_cast<std::uint8_t>(color.value());
            ev.thickness = static_cast<std::uint8_t>(thickness.value());
            ev.start = start.value();
            return ServerMessage{ev};
        }
        if (type.value() == "stroke_point") {
            StrokePointData ev;
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            auto seq = getRequiredNumber<Seq>(object, "seq");
            if (!seq) {
                return seq.error();
            }
            auto author = getRequiredNumber<ClientId>(object, "author");
            if (!author) {
                return author.error();
            }
            auto id = getRequiredNumber<StrokeId>(object, "id");
            if (!id) {
                return id.error();
            }
            auto point = getRequiredPoint(object, "point");
            if (!point) {
                return point.error();
            }
            ev.room = room.value();
            ev.seq = seq.value();
            ev.author = author.value();
            ev.id = id.value();
            ev.point = point.value();
            return ServerMessage{ev};
        }
        if (type.value() == "stroke_end") {
            StrokeEndData ev;
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            auto seq = getRequiredNumber<Seq>(object, "seq");
            if (!seq) {
                return seq.error();
            }
            auto author = getRequiredNumber<ClientId>(object, "author");
            if (!author) {
                return author.error();
            }
            auto id = getRequiredNumber<StrokeId>(object, "id");
            if (!id) {
                return id.error();
            }
            ev.room = room.value();
            ev.seq = seq.value();
            ev.author = author.value();
            ev.id = id.value();
            return ServerMessage{ev};
        }
        if (type.value() == "cursor_move") {
            CursorMoveData ev;
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            auto seq = getRequiredNumber<Seq>(object, "seq");
            if (!seq) {
                return seq.error();
            }
            auto author = getRequiredNumber<ClientId>(object, "author");
            if (!author) {
                return author.error();
            }
            auto point = getRequiredPoint(object, "point");
            if (!point) {
                return point.error();
            }
            ev.room = room.value();
            ev.seq = seq.value();
            ev.author = author.value();
            ev.point = point.value();
            return ServerMessage{ev};
        }
        if (type.value() == "clear") {
            ClearData ev;
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            auto seq = getRequiredNumber<Seq>(object, "seq");
            if (!seq) {
                return seq.error();
            }
            auto author = getRequiredNumber<ClientId>(object, "author");
            if (!author) {
                return author.error();
            }
            ev.room = room.value();
            ev.seq = seq.value();
            ev.author = author.value();
            return ServerMessage{ev};
        }
        if (type.value() == "member_left") {
            MemberLeftData ev;
            auto room = getRequiredNumber<RoomId>(object, "room");
            if (!room) {
                return room.error();
            }
            auto seq = getRequiredNumber<Seq>(object, "seq");
            if (!seq) {
                return seq.error();
            }
            auto client = getRequiredNumber<ClientId>(object, "client");
            if (!client) {
                return client.error();
            }
            ev.room = room.value();
            ev.seq = seq.value();
            ev.client = client.value();
            return ServerMessage{ev};
        }

        return QString("unknown message type: %1").arg(type.value());
    }

    std::string makePingRequest() {
        return dumpWithNewline(json{{"type", "ping"}});
    }

    std::string makeCreateRoomRequest() {
        return dumpWithNewline(json{{"type", "create_room"}});
    }

    std::string makeJoinRoomRequest(RoomId room) {
        return dumpWithNewline(json{{"type", "join_room"}, {"room", room}});
    }

    std::string makeLeaveRoomRequest(RoomId room) {
        return dumpWithNewline(json{{"type", "leave_room"}, {"room", room}});
    }

    std::string makeStrokeBeginRequest(RoomId room, std::uint8_t color, std::uint8_t thickness, const QPointF& start) {
        return dumpWithNewline(json{
                {"type", "stroke_begin"},
                {"room", room},
                {"color", color},
                {"thickness", thickness},
                {"start", makePointJson(start)}
        });
    }

    std::string makeStrokePointRequest(RoomId room, const QPointF& point) {
        return dumpWithNewline(json{{"type", "stroke_point"}, {"room", room}, {"point", makePointJson(point)}});
    }

    std::string makeStrokeEndRequest(RoomId room) {
        return dumpWithNewline(json{{"type", "stroke_end"}, {"room", room}});
    }

    std::string makeCursorMoveRequest(RoomId room, const QPointF& point) {
        return dumpWithNewline(json{{"type", "cursor_move"}, {"room", room}, {"point", makePointJson(point)}});
    }

    std::string makeClearRequest(RoomId room) {
        return dumpWithNewline(json{{"type", "clear"}, {"room", room}});
    }

}