#include "media_messages.h"
#include "json_message.h"
#include "utils.h"
#include <boost\lexical_cast.hpp>
#include <map>

namespace chromecast
{
	static std::map<std::string, MediaStatus::ePlayerState> player_json_state_to_state = 
	{
		{ "PLAYING", MediaStatus::ePlayerState::Playing },
		{ "BUFFERING", MediaStatus::ePlayerState::Buffering },
		{ "PAUSED", MediaStatus::ePlayerState::Paused },
		{ "IDLE", MediaStatus::ePlayerState::Idle }
	};

	static std::map<std::string, MediaStatus::eRepeatMode> player_json_repeat_mode_to_repeat_mode =
	{
		{ "REPEAT_OFF", MediaStatus::eRepeatMode::Off },
	};

	void Media::Track::ToMessage(JsonMessagePart& message) const
	{
		message["trackId"] = id;
		if (type == eTrackType::Text)
			message["type"] = "TEXT";
		else
			THROW_ON_ERROR_EX(true, "unknown track type");

		message["trackContentId"] = content_id;
		message["trackContentType"] = content_type;
		message["name"] = name;
		message["language"] = language;
		message["subtype"] = sub_type;
	}

	Media::Track Media::Track::FromMessage(const ConstJsonMessagePart& message)
	{
		eTrackType type = eTrackType::Text;
		std::string track_type = message["type"].GetString();
		if (track_type != "TEXT")
			type = eTrackType::Unknown;

		Track track;
		track.id = message["trackId"].GetUint32();
		track.type = type;
		track.content_id = message["trackContentId"].GetString();
		track.content_type = message["trackContentType"].GetString();
		track.name = message["name"].GetString();
		track.language = message["language"].GetString();
		track.sub_type = message["subtype"].GetString();
		return track;
	}

	Media::TextTrackStyle::Color Media::TextTrackStyle::Color::FromString(const std::string& value)
	{
		if (value.length() != 9)
			throw std::runtime_error("invalid color value " + value);

		if (value.front() != '#')
			throw std::runtime_error("color value does not start with a # prefix" + value);

		size_t pos = 1;
		const size_t color_channel_string_length = 2;
		Color color;
		for (auto& b : color.value.argb)
		{
			b = boost::lexical_cast<byte>(value.substr(pos, color_channel_string_length));
			pos += color_channel_string_length;
		}
		return color;
	}

	Media::TextTrackStyle::Edge Media::TextTrackStyle::Edge::FromMessage(const ConstJsonMessagePart& message)
	{
		eType type = eType::Outline;
		std::string edge_type = message["edgeType"].GetString();
		if (edge_type != "OUTLINE")
			type = eType::Outline;

		Edge edge;
		edge.type = type;
		edge.color = Color::FromString(message["edgeColor"].GetString());
		return edge;
	}

	Media::TextTrackStyle::Font Media::TextTrackStyle::Font::FromMessage(const ConstJsonMessagePart& message)
	{
		eStyle style = eStyle::Normal;
		std::string font_style = message["fontStyle"].GetString();
		if (font_style != "NORMAL")
			style = eStyle::Unknown;

		Font font;
		font.scale = message["fontScale"].GetDouble();
		font.style = style;
		font.family = message["fontFamily"].GetString();
		font.generic_family = message["fontGenericFamily"].GetString();
		return font;
	}

	Media::TextTrackStyle::Window Media::TextTrackStyle::Window::FromMessage(const ConstJsonMessagePart& message)
	{
		Window window;
		window.color = Color::FromString(message["windowColor"].GetString());
		window.rounded_corner_radius = message["windowRoundedCornerRadius"].GetDouble();
		window.type = Window::eType::None;
		return window;
	}

	Media::TextTrackStyle Media::TextTrackStyle::FromMessage(const ConstJsonMessagePart& message)
	{
		TextTrackStyle text_track_style;
		text_track_style.background = Color::FromString(message["backgroundColor"].GetString());
		text_track_style.foreground = Color::FromString(message["foregroundColor"].GetString());
		text_track_style.edge = Edge::FromMessage(message);
		text_track_style.font = Font::FromMessage(message);
		text_track_style.window = Window::FromMessage(message);
		return text_track_style;
	}

	Media::MetaData::Image::Image(std::string&& url)
		: url(url)
	{

	}

	void Media::MetaData::Image::ToMessage(JsonMessagePart& message) const
	{
		message["url"] = url;
	}

	Media::MetaData::Image Media::MetaData::Image::FromMessage(const ConstJsonMessagePart& message)
	{
		return Image(message["url"].GetString());
	}

	void Media::MetaData::ToMessage(JsonMessagePart& message) const
	{
		auto& metadata_part = message["metadata"];
		metadata_part["type"] = type;
		metadata_part["metadataType"] = (uint32_t)metadataType;
		metadata_part["title"] = title;
		auto& images_part = metadata_part["images"];
		for (auto& image : images)
			images_part["url"] = image.url;
	}

	Media::MetaData Media::MetaData::FromMessage(const ConstJsonMessagePart& message)
	{
		auto& meta_data_part = message["metadata"];
		MetaData meta_data;
		meta_data.type = meta_data_part["type"].GetUint32();
		meta_data.title = meta_data_part["title"].GetString();
		meta_data.metadataType = static_cast<eMetadataType>(meta_data_part["metadataType"].GetUint32());

		auto& images_part = meta_data_part["images"];
		for (size_t index = 0; index < images_part.Size(); ++index)
			meta_data.images.push_back(Image::FromMessage(images_part[index]));
		return meta_data;
	}

	void Media::ToMessage(JsonMessagePart& message) const
	{
		auto& json_media = message["media"];
		json_media["contentId"] = content_id;
		json_media["contentType"] = content_type;
		json_media["streamType"] = stream_type == Media::eStreamType::BUFFERED ? "BUFFERED" : "LIVE";
		json_media["tracks"] = tracks;
		meta_data.ToMessage(message);
	}

	Media Media::FromMessage(const ConstJsonMessagePart& message)
	{
		Media media;
		media.content_id = message["contentId"].GetString();
		media.content_type = message["contentType"].GetString();

		if (message.HasMember("tracks"))
		{
			auto& tracks_part = message["tracks"];
			for (size_t index = 0; index < tracks_part.Size(); ++index)
				media.tracks.push_back(Track::FromMessage(tracks_part[index]));
		}
		if (message.HasMember("textTrackStyle"))
			media.text_track_style = TextTrackStyle::FromMessage(message);
		if (message.HasMember("metadata"))
			media.meta_data = MetaData::FromMessage(message);
		return media;
	}

	MediaItem MediaItem::FromMessage(const ConstJsonMessagePart& message)
	{
		MediaItem item;
		item.id = message["itemId"].GetUint32();
		item.autoplay = message["autoplay"].GetBool();
		item.start_time = message["startTime"].GetDouble();
		auto& active_track_ids_message_part = message["activeTrackIds"];
		for (size_t index = 0; index < active_track_ids_message_part.Size(); ++index)
			item.active_track_ids.push_back(active_track_ids_message_part[index].GetUint32());
		item.media = Media::FromMessage(message["media"]);
		return item;
	}

	MediaStatus MediaStatus::FromMessage(const ConstJsonMessagePart& message)
	{
		std::string type = message["type"].GetString();
		THROW_ON_ERROR_EX(type != "MEDIA_STATUS", "invalid message type, expected a receiver status message");

		auto& json_statuses = message["status"];
		std::vector<MediaStatus> statuses;
		for (size_t index = 0; index < json_statuses.Size(); ++index)
		{
			auto& json_status = json_statuses[index];
			MediaStatus status;
			status.valid = true;
			status.session_id = json_status["mediaSessionId"].GetUint32();
			status.playback_rate = json_status["playbackRate"].GetUint32();
			status.current_time = json_status["currentTime"].GetDouble();
			status.supported_media_commands = (eSupportedCommands)(json_status["supportedMediaCommands"].GetUint32());

			auto& volume = json_status["volume"];
			status.muted = volume["muted"].GetBool();
			status.volume_level = volume["level"].GetDouble();
			status.current_item_id = json_status["currentItemId"].GetUint32();

			std::string media_play_state = json_status["playerState"].GetString();
			auto it_player_state = player_json_state_to_state.find(media_play_state);
			THROW_ON_ERROR_EX(it_player_state == player_json_state_to_state.end(), "unrecognized play state " + media_play_state);
			status.player_state = it_player_state->second;

			if (json_status.HasMember("repeatMode"))
			{
				std::string media_repeat_mode = json_status["repeatMode"].GetString();
				auto it_repeat_mode = player_json_repeat_mode_to_repeat_mode.find(media_repeat_mode);
				THROW_ON_ERROR_EX(it_repeat_mode == player_json_repeat_mode_to_repeat_mode.end(), "unrecognized repeat mode " + media_repeat_mode);
				status.repeat_mode = it_repeat_mode->second;
			}
			if (json_status.HasMember("media"))
				status.media = Media::FromMessage(json_status["media"]);
			if (json_status.HasMember("items"))
			{
				auto& items = json_status["items"];
				for (size_t index = 0; index < items.Size(); ++index)
					status.items.push_back(MediaItem::FromMessage(items[index]));
			}

			statuses.push_back(status);
		}

		MediaStatus status;
		if (!statuses.empty())
			status = statuses.front();
		return status;
	}

	bool MediaResponse::Succeeded() const
	{
		return reason.empty();
	}

	bool MediaResponse::Failed() const
	{
		return !Succeeded();
	}

	void MediaResponse::ThrowOnFailure() const
	{
		if (Failed())
			throw std::runtime_error(reason);
	}

	std::string MediaResponse::ToString() const
	{
		return reason.empty() ? "success" : "failure, " + reason;
	}

	const MediaStatus& MediaResponse::GetStatus() const
	{
		return status;
	}

	MediaResponse MediaResponse::FromMessage(const ConstJsonMessagePart& message)
	{
		MediaResponse result;
		if (message.HasMember("status"))
			result.status = MediaStatus::FromMessage(message);
		else
			result.reason = message["reason"].GetString();
		return result;
	}
};