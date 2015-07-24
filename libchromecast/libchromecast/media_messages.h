#pragma once
#include "types.h"
#include <string>
#include <vector>

namespace chromecast
{
	class JsonMessagePart;
	class ConstJsonMessagePart;
	struct Media
	{
		struct Track
		{
			enum class eTrackType : uint32_t
			{
				Unknown,
				Text
			};

			uint32_t id;
			eTrackType type;

			std::string content_id;
			std::string content_type;
			std::string name;
			std::string language;
			std::string sub_type;

			void ToMessage(JsonMessagePart& message) const;
			static Track FromMessage(const ConstJsonMessagePart& message);
		};

		struct TextTrackStyle
		{
			struct Color
			{
				union
				{
					byte argb[4];
					uint32_t raw;
				} value;

				void ToMessage(JsonMessagePart& message) const;
				static Color FromString(const std::string& value);
			};

			struct Edge
			{
				enum class eType
				{
					Unknown,
					Outline
				};

				eType type;
				Color color;

				void ToMessage(JsonMessagePart& message) const;
				static Edge FromMessage(const ConstJsonMessagePart& message);
			};

			struct Font
			{
				enum class eStyle
				{
					Unknown,
					Normal
				};

				double scale;
				eStyle style;
				std::string family;
				std::string generic_family;

				void ToMessage(JsonMessagePart& message) const;
				static Font FromMessage(const ConstJsonMessagePart& message);
			};

			struct Window
			{
				enum class eType
				{
					None
				};

				eType type;
				Color color;
				double rounded_corner_radius;

				void ToMessage(JsonMessagePart& message) const;
				static Window FromMessage(const ConstJsonMessagePart& message);
			};

			Color background;
			Color foreground;
			Edge edge;
			Font font;
			Window window;

			void ToMessage(JsonMessagePart& message) const;
			static TextTrackStyle FromMessage(const ConstJsonMessagePart& message);
		};

		struct MetaData
		{
			enum class eMetadataType : uint32_t
			{
				Generic = 0,
				TVShow = 1,
				Movie = 2,
				MusicTrack = 3,
				Photo = 4
			};

			struct Image
			{
				std::string url;

				Image(std::string&& url);

				void ToMessage(JsonMessagePart& message) const;
				static Image FromMessage(const ConstJsonMessagePart& message);
			};

			uint32_t type = 0;
			eMetadataType metadataType = eMetadataType::Generic;
			std::string title;
			std::vector<Image> images;

			void ToMessage(JsonMessagePart& message) const;
			static MetaData FromMessage(const ConstJsonMessagePart& message);
		};

		enum class eStreamType
		{
			BUFFERED,
			LIVE
		};

		MetaData meta_data;
		std::string content_id;
		std::string content_type;
		double duration;
		eStreamType stream_type;
		std::vector<Track> tracks;
		TextTrackStyle text_track_style;

		void ToMessage(JsonMessagePart& message) const;
		static Media FromMessage(const ConstJsonMessagePart& message);
	};

	struct MediaItem
	{
		uint32_t id;
		bool autoplay;
		double start_time;
		std::vector<uint32_t> active_track_ids;
		Media media;

		static MediaItem FromMessage(const ConstJsonMessagePart& message);
	};

	struct MediaStatus
	{
		enum class ePlayerState
		{
			Idle,
			Buffering,
			Playing,
			Paused,
			Missing
		};

		enum class eRepeatMode
		{
			Off,
			Missing
		};

		enum eSupportedCommands
		{
			commandNone = 0,
			commandPause = (1 << 0),
			commandSeek = (1 << 1),
			commandVolume = (1 << 2),
			commandMute = (1 << 3),
			commandSkipForward = (1 << 4),
			commandSkipBackward = (1 << 5),
		};

		bool valid = false;
		bool muted = false;
		double volume_level = 0;
		double current_time = 0;
		uint32_t session_id = 0;
		uint32_t playback_rate = 0;
		uint32_t current_item_id = 0;
		ePlayerState player_state = ePlayerState::Missing;
		eSupportedCommands supported_media_commands = commandNone;
		eRepeatMode repeat_mode = eRepeatMode::Missing;
		Media media;
		std::vector<MediaItem> items;

		static MediaStatus FromMessage(const ConstJsonMessagePart& message);
	};

	class MediaResponse
	{
		std::string reason;
		MediaStatus status;
	public:
		bool Succeeded() const;
		bool Failed() const;
		void ThrowOnFailure() const;

		std::string ToString() const;
		const MediaStatus& GetStatus() const;

		static MediaResponse FromMessage(const ConstJsonMessagePart& message);
	};
}