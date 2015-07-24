#pragma once
#include "sender_application.h"
#include "media_channel.h"

namespace chromecast
{
	template <typename TMediaChannel = MediaChannel>
	class DefaultMediaPlayer : public SenderApplication
	{
		std::string _sender_id;
		std::unique_ptr<TMediaChannel> _media_channel;

		void Initialize(ChromecastChannelFactory& channel_factory, const ReceiverStatus::ApplicationInfo& app_info, const std::function<void(bool)>& on_initialization_completed)
		{
			__super::Initialize(channel_factory, app_info, on_initialization_completed);
			_media_channel->GetStatus([=](const MediaResponse& status)
			{
				if (on_initialization_completed)
					on_initialization_completed(status.Succeeded());
			});
		}

		void CreateChannels(ChromecastChannelFactory& channel_factory, const std::string& sender_id, const std::string& receiver_id)
		{
			__super::CreateChannels(channel_factory, sender_id, receiver_id);
			if (_sender_id.empty())
				_sender_id = sender_id;
			_media_channel = channel_factory.CreateIOServiceChannel<TMediaChannel>(_sender_id, receiver_id);
		}

		void OnStopped()
		{
			_media_channel.reset();
		}

		void EnsureChannelExists() const
		{
			if (!_media_channel)
				throw std::runtime_error("missing media channel");
		}
	public:
		DefaultMediaPlayer(std::string sender_id = "")
			: SenderApplication("CC1AD845"),
			_sender_id(sender_id)
		{

		}

		void Load(const Media& media, bool autoplay, const MediaChannel::MediaOperationCallback& callback)
		{
			EnsureChannelExists();
			_media_channel->Load(media, autoplay, callback);
		}

		void Play(const MediaChannel::MediaOperationCallback& callback)
		{
			EnsureChannelExists();
			_media_channel->Play(callback);
		}

		void Pause(const MediaChannel::MediaOperationCallback& callback)
		{
			EnsureChannelExists();
			_media_channel->Pause(callback);
		}

		void Stop(const MediaChannel::MediaOperationCallback& callback)
		{
			EnsureChannelExists();
			_media_channel->Stop(callback);
		}

		void Seek(uint32_t minute_offset, const MediaChannel::MediaOperationCallback& callback)
		{
			EnsureChannelExists();
			_media_channel->Seek(minute_offset, callback);
		}
	};
}