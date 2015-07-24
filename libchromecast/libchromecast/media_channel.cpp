#include "json_message.h"
#include "media_channel.h"

namespace chromecast
{
	using namespace std;

	static std::string GetMediaPlayerNamespace()
	{
		return "urn:x-cast:com.google.cast.media";
	}

	bool MediaChannel::OnResponse(uint64_t request_id, const JsonMessage& message)
	{
		if (message["type"].GetString() == "MEDIA_STATUS")
			_last_status = MediaStatus::FromMessage(message);

		if (request_id == 0)
			return true;

		return __super::OnResponse(request_id, message);
	}

	void MediaChannel::SessionRequest(JsonMessage&& message, const MediaOperationCallback& callback)
	{
		message["mediaSessionId"] = _last_status.session_id;
		Request<MediaResponse>(move(message), [=](const MediaResponse& result)
		{
			if (callback)
				callback(result);
		});
	}

	MediaChannel::MediaChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const std::string& sender_id, const std::string& receiver_id)
		: RequestChannel(io_service, connection, ChromecastChannel::Address(sender_id, receiver_id, GetMediaPlayerNamespace()))
	{

	}

	void MediaChannel::Load(const Media& media, bool autoplay, const MediaOperationCallback& callback)
	{
		JsonMessage message;
		message["type"] = "LOAD";
		media.ToMessage(message);

		Request<MediaResponse>(move(message), [=](const MediaResponse& result)
		{
			if (result.Succeeded())
				_last_status = result.GetStatus();
			if (callback)
				callback(result);
		});
	}

	void MediaChannel::Play(const MediaOperationCallback& callback)
	{
		JsonMessage message;
		message["type"] = "PLAY";
		SessionRequest(move(message), callback);
	}

	void MediaChannel::Pause(const MediaOperationCallback& callback)
	{
		JsonMessage message;
		message["type"] = "PAUSE";
		SessionRequest(move(message), callback);
	}

	void MediaChannel::Stop(const MediaOperationCallback& callback)
	{
		JsonMessage message;
		message["type"] = "STOP";
		SessionRequest(move(message), callback);
	}

	void MediaChannel::Seek(uint32_t minute_offset, const MediaOperationCallback& callback)
	{
		JsonMessage message;
		message["type"] = "SEEK";
		message["currentTime"] = minute_offset;
		SessionRequest(move(message), callback);
	}

	void MediaChannel::SetTrackInfo(const std::vector<std::string>& track_ids, const MediaOperationCallback& callback)
	{
		JsonMessage message;
		message["type"] = "EDIT_TRACKS_INFO";
		message["activeTrackIds"] = track_ids;
		SessionRequest(move(message), callback);
	}

	void MediaChannel::GetStatus(const MediaOperationCallback& callback)
	{
		JsonMessage message;
		message["type"] = "GET_STATUS";
		Request<MediaResponse>(move(message), callback);
	}
}