// chormecast-sample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "../libchromecast/client.h"
#include "../libchromecast/default_media_player.h"

static void Log(const std::string& message)
{
	std::cout << message << std::endl;
}

template <typename TChannel>
struct PrintingChannel : public TChannel
{
	template <typename... TArgs>
	PrintingChannel(TArgs&&... Args)
		: TChannel(Args...)
	{
	}

	void OnMessage(const chromecast::CastMessage& message)
	{
		Log("Receiving:");
		Log(message.ToString());
		ChromecastChannel::OnMessage(message);
	}

	void OnSending(const chromecast::CastMessage& message) override
	{
		Log("Sending:");
		Log(message.ToString());
	}
};

typedef PrintingChannel<chromecast::ReceiverChannel> PrintingReceiverChannel;
typedef PrintingChannel<chromecast::HeartbeatChannel> PrintingHeartbeatChannel;
typedef PrintingChannel<chromecast::ConnectionChannel> PrintingConnectionChannel;
typedef PrintingChannel<chromecast::MediaChannel> PrintingMediaChannel;

typedef chromecast::DefaultMediaPlayer<PrintingMediaChannel> MediaPlayer;
typedef chromecast::MediaResponse MediaResponse;

template <typename TConnection, typename TReceiverChannel = chromecast::ReceiverChannel, typename TConnectionChannel = chromecast::ConnectionChannel, typename THeartbeatChannel = chromecast::HeartbeatChannel>
struct CommandBasedClient : public chromecast::ChromecastClient<TConnection, TReceiverChannel, TConnectionChannel, THeartbeatChannel>
{
public:
	CommandBasedClient(boost::asio::io_service& io_service)
		: chromecast::ChromecastClient<TConnection, TReceiverChannel, TConnectionChannel, THeartbeatChannel>(io_service)
	{

	}

	void OnCommand(const std::string& command, const std::string& param)
	{
		if (command == "connect")
			AsyncConnect(param, [=](ReceiverChannel& receiver)
			{
				std::cout << "connected to " << param << std::endl;
			});
		else if (command == "quit")
			Close();
		else if (command == "mute")
		{
			Mute(param != "false", [=](MediaResponse muted)
			{
				std::cout << "muted volume: " << muted.GetMessage() << std::endl;
			});
		}
		else if (command == "set volume")
		{
			double volume = boost::lexical_cast<double>(param);
			if (volume < 0 || volume > 1)
				throw std::runtime_error("invalid volume param, must be between 0 and 1");
			SetVolume(volume, [=](bool success)
			{
				std::cout << "changed volume level: " << success << std::endl;
			});
		}
		else if (command == "play mp4")
		{
			Launch(std::make_unique<MediaPlayer>(), [=](MediaPlayer& player)
			{
				Media media;
				media._content_id = param;
				media._content_type = "video/mp4";
				media._stream_type = Media::eStreamType::BUFFERED;
				player.Load(media, true, [=](bool loaded)
				{
					std::cout << "loaded: " << param << std::endl;
				});
			});
		}
	}
};

struct MyConnection : public chromecast::ChromecastConnection
{
	MyConnection(boost::asio::io_service& service)
		: ChromecastConnection(service)
	{

	}

	void OnUnrecognizedAddress(const chromecast::CastMessage& message)
	{
		if (message.address._source != message.address._destination || message.address._source != "Tr@n$p0rt-0")
			return __super::OnUnrecognizedAddress(message);

		std::cout << "encountered the unknown Tr@n$p0rt message, dropping it quietly for now." << std::endl;
	}
};

typedef chromecast::ChromecastClient<MyConnection> ChromecastClient;
typedef chromecast::ChromecastClient<MyConnection, PrintingReceiverChannel, PrintingConnectionChannel, PrintingHeartbeatChannel> DebugChromecastClient;


static void InitializeClientA(const std::string& device_ip, std::shared_ptr<ChromecastClient> client)
{
	client->AsyncConnect(device_ip, [=](bool connected)
	{
		std::cout << "connected: " << connected << std::endl;
		if (!connected)
			return;

		std::cout << "muting volume..." << std::endl;
		/*client->Mute(true, [=](bool muted)
		{
			std::cout << "muted volume: " << muted << ", launching application" << std::endl;
			client->Launch<MediaPlayer>([&](MediaPlayer& player)
			{
				std::cout << "launched default media player" << std::endl;
				chromecast::Media media;
				media.content_id = "http://commondatastorage.googleapis.com/gtv-videos-bucket/big_buck_bunny_1080p.mp4";
				media.content_type = "video/mp4";
				media.stream_type = chromecast::Media::eStreamType::BUFFERED;
				media.meta_data.images.emplace_back("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/images/BigBuckBunny.jpg");
				player.Load(media, false, [&](const MediaResponse& loaded)
				{
					std::cout << "loaded media" << loaded.ToString() << std::endl;
					if (loaded.Failed())
						return;

					player.Pause([&](const MediaResponse& paused)
					{
						std::cout << "paused media" << paused.ToString() << std::endl;
						player.Play([&](const MediaResponse& playing)
						{
							std::cout << "playing media" << playing.ToString() << std::endl;
							player.Seek(1, [&](const MediaResponse& seeked)
							{
								std::cout << "seeked media " << seeked.ToString() << std::endl;
								player.Stop([&](const MediaResponse& stopped)
								{
									std::cout << "stopped media " << stopped.ToString() << std::endl;
									client->Mute(false, [=](bool muted)
									{
										std::cout << "mute: " << muted << std::endl;
										std::cout << "closing connection" << std::endl;
										client->Close();
									});
								});
							});
						});
					});
				});
			});
		});*/
	});
}

static void InitializeClientB(const std::string& device_ip, std::shared_ptr<ChromecastClient> client)
{
	client->AsyncConnect(device_ip, [=](bool connected)
	{
		std::cout << "Connected: " << connected << std::endl;
		auto player = std::make_shared<MediaPlayer>();
		std::cout << "trying to join with the default media player..." << std::endl;
		client->Join(player, [=](bool joined)
		{
			std::cout << "Joined: " << joined << std::endl;
			if (!joined)
				return;

			std::cout << "Trying to pause running video from another client..." << std::endl;
			player->Pause([=](const MediaResponse& paused)
			{
				std::cout << "paused: " << paused.ToString() << std::endl;
				player->Play([=](const MediaResponse& played)
				{
					std::cout << "played: " << played.ToString() << std::endl;
				});
			});
		});
	});
}

// wide char to multi byte:
std::string ws2s(const std::wstring& wstr)
{
	int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), 0, 0, 0, 0);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), &strTo[0], size_needed, 0, 0);
	return strTo;
}

int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: " << argv[0] << " <device-ip>\n" << std::endl;
			return 1;
		}

		std::string device_ip = ws2s(argv[1]);

		boost::asio::io_service io_service;
		std::shared_ptr<ChromecastClient> client = std::make_shared<ChromecastClient>(io_service);
		//std::shared_ptr<DebugChromecastClient> client = std::make_shared<DebugChromecastClient>(io_service);
		InitializeClientA(device_ip, client);

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Error:\n" << e.what() << "\n";
	}
}

