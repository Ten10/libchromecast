#pragma once
#include "receiver_channel.h"
#include "media_messages.h"

namespace chromecast
{
	class MediaChannel : public RequestChannel
	{
		MediaStatus _last_status;
	public:
		typedef std::function<void(MediaResponse)> MediaOperationCallback;
	protected:
		bool OnResponse(uint64_t request_id, const JsonMessage& message);
		void SessionRequest(JsonMessage&& message, const MediaOperationCallback& callback);
	public:
		MediaChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const std::string& sender_id, const std::string& receiver_id);

		void Load(const Media& media, bool autoplay, const MediaOperationCallback& callback);
		void Play(const MediaOperationCallback& callback);
		void Pause(const MediaOperationCallback& callback);
		void Stop(const MediaOperationCallback& callback);
		void Seek(uint32_t minute_offset, const MediaOperationCallback& callback);
		void SetTrackInfo(const std::vector<std::string>& track_ids, const MediaOperationCallback& callback);
		void GetStatus(const MediaOperationCallback& callback);
	};
}