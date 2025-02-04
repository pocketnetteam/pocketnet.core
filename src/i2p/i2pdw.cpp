// Copyright (c) 2025-2025 The Pocketcoin Core developers
// Licensed under the Apache License, Version 2.0;
// http://www.apache.org/licenses/LICENSE-2.0

#include "i2pdw.h"
#include "logging.h"
#include "util/system.h"
#include <libi2pd/NetDb.hpp>
#include <libi2pd/Log.h>
#include <libi2pd/Config.h>
#include <libi2pd/Transports.h>
#include <libi2pd/Tunnel.h>
#include <libi2pd/ClientContext.h>

namespace i2pdw {

	DaemonWrapper::DaemonWrapper() {
	}

	DaemonWrapper::~DaemonWrapper() {
	}


	bool DaemonWrapper::Init()
	{
		i2p::config::Init();
		i2p::config::ParseCmdline(1, {});

		std::string config; i2p::config::GetOption("conf", config);
		std::string datadir = (GetDataDir() / "i2pd").string();
		
		i2p::fs::DetectDataDir(datadir, false);
		i2p::fs::Init();

		datadir = i2p::fs::GetDataDir();

		if (config == "")
		{
			config = i2p::fs::DataDirPath("i2pd.conf");
			if (!i2p::fs::Exists(config)) {
				// use i2pd.conf only if exists
				config = ""; /* reset */
			}
		}

		i2p::config::ParseConfig(config);
		i2p::config::Finalize();

		std::string certsdir; i2p::config::GetOption("certsdir", certsdir);
		i2p::fs::SetCertsDir(certsdir);

		certsdir = i2p::fs::GetCertsDir();

		std::string logs     = ""; i2p::config::GetOption("log",        logs);
		std::string logfile  = ""; i2p::config::GetOption("logfile",    logfile);
		std::string loglevel = ""; i2p::config::GetOption("loglevel",   loglevel);
		bool logclftime;           i2p::config::GetOption("logclftime", logclftime);

// 		/* setup logging */
// 		if (logclftime)
// 			i2p::log::Logger().SetTimeFormat ("[%d/%b/%Y:%H:%M:%S %z]");

// #ifdef WIN32_APP
// 		// Win32 app with GUI supports only logging to file
// 		logs = "file";
// #else
// 		if ((logs == "" || logs == "stdout"))
// 			logs = "file";
// #endif

// 		i2p::log::Logger().SetLogLevel(loglevel);
// 		if (logstream) {
// 			LogPrintCategory(eLogInfo, "Log: Sending messages to std::ostream");
// 			i2p::log::Logger().SendTo (logstream);
// 		} else if (logs == "file") {
// 			if (logfile == "")
// 				logfile = i2p::fs::DataDirPath("i2pd.log");
// 			LogPrintCategory(eLogInfo, "Log: Sending messages to ", logfile);
// 			i2p::log::Logger().SendTo (logfile);
// #ifndef _WIN32
// 		} else if (logs == "syslog") {
// 			LogPrintCategory(eLogInfo, "Log: Sending messages to syslog");
// 			i2p::log::Logger().SendTo("i2pd", LOG_DAEMON);
// #endif
// 		} else {
// 			// use stdout -- default
// 		}

		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "i2pd v%s (%s) starting...\n", VERSION, I2P_VERSION);
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "FS: Main config file: %s\n", config);
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "FS: Data directory: %s\n", datadir);
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "FS: Certificates directory: %s\n", certsdir);

		bool precomputation; i2p::config::GetOption("precomputation.elgamal", precomputation);
		bool ssu; i2p::config::GetOption("ssu", ssu);
		if (!ssu && i2p::config::IsDefault ("precomputation.elgamal"))
			precomputation = false; // we don't elgamal table if no ssu, unless it's specified explicitly
		i2p::crypto::InitCrypto (precomputation);

		i2p::transport::InitAddressFromIface (); // get address4/6 from interfaces

		int netID; i2p::config::GetOption("netid", netID);
		i2p::context.SetNetID (netID);

		bool checkReserved; i2p::config::GetOption("reservedrange", checkReserved);
		i2p::transport::transports.SetCheckReserved(checkReserved);

		i2p::context.Init ();

		i2p::transport::InitTransports ();

		bool isFloodfill; i2p::config::GetOption("floodfill", isFloodfill);
		if (isFloodfill)
		{
			LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Router configured as floodfill\n");
			i2p::context.SetFloodfill (true);
		}
		else
			i2p::context.SetFloodfill (false);

		bool transit; i2p::config::GetOption("notransit", transit);
		i2p::context.SetAcceptsTunnels (!transit);
		uint32_t transitTunnels; i2p::config::GetOption("limits.transittunnels", transitTunnels);
		if (isFloodfill && i2p::config::IsDefault ("limits.transittunnels"))
			transitTunnels *= 2; // double default number of transit tunnels for floodfill
		i2p::tunnel::tunnels.SetMaxNumTransitTunnels (transitTunnels);

		/* this section also honors 'floodfill' flag, if set above */
		std::string bandwidth; i2p::config::GetOption("bandwidth", bandwidth);
		if (bandwidth.length () > 0)
		{
			if (bandwidth.length () == 1 && ((bandwidth[0] >= 'K' && bandwidth[0] <= 'P') || bandwidth[0] == 'X' ))
			{
				i2p::context.SetBandwidth (bandwidth[0]);
				LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Bandwidth set to %d KBps\n", i2p::context.GetBandwidthLimit ());
			}
			else
			{
				auto value = std::atoi(bandwidth.c_str());
				if (value > 0)
				{
					i2p::context.SetBandwidth (value);
					LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Bandwidth set to %d KBps\n", i2p::context.GetBandwidthLimit ());
				}
				else
				{
					LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Unexpected bandwidth %s. Set to 'low'\n", bandwidth);
					i2p::context.SetBandwidth (i2p::data::CAPS_FLAG_LOW_BANDWIDTH2);
				}
			}
		}
		else if (isFloodfill)
		{
			LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Floodfill bandwidth set to 'extra'\n");
			i2p::context.SetBandwidth (i2p::data::CAPS_FLAG_EXTRA_BANDWIDTH2);
		}
		else
		{
			LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: bandwidth set to 'low'\n");
			i2p::context.SetBandwidth (i2p::data::CAPS_FLAG_LOW_BANDWIDTH2);
		}

		int shareRatio; i2p::config::GetOption("share", shareRatio);
		i2p::context.SetShareRatio (shareRatio);

		std::string family; i2p::config::GetOption("family", family);
		i2p::context.SetFamily (family);
		if (family.length () > 0)
			LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Router family set to %s\n", family);

		bool trust; i2p::config::GetOption("trust.enabled", trust);
		if (trust)
		{
			LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Explicit trust enabled\n");
			std::string fam; i2p::config::GetOption("trust.family", fam);
			std::string routers; i2p::config::GetOption("trust.routers", routers);
			bool restricted = false;
			if (fam.length() > 0)
			{
				std::set<std::string> fams;
				size_t pos = 0, comma;
				do
				{
					comma = fam.find (',', pos);
					fams.insert (fam.substr (pos, comma != std::string::npos ? comma - pos : std::string::npos));
					pos = comma + 1;
				}
				while (comma != std::string::npos);
				i2p::transport::transports.RestrictRoutesToFamilies(fams);
				restricted = fams.size() > 0;
			}
			if (routers.length() > 0) {
				std::set<i2p::data::IdentHash> idents;
				size_t pos = 0, comma;
				do
				{
					comma = routers.find (',', pos);
					i2p::data::IdentHash ident;
					ident.FromBase64 (routers.substr (pos, comma != std::string::npos ? comma - pos : std::string::npos));
					idents.insert (ident);
					pos = comma + 1;
				}
				while (comma != std::string::npos);
				LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Setting restricted routes to use %d trusted routers\n", idents.size());
				i2p::transport::transports.RestrictRoutesToRouters(idents);
				restricted = idents.size() > 0;
			}
			if(!restricted)
				LogPrintLevel(BCLog::Level::Error, BCLog::I2P, "Daemon: No trusted routers of families specified\n");
		}

		bool hidden; i2p::config::GetOption("trust.hidden", hidden);
		if (hidden)
		{
			LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Hidden mode enabled\n");
			i2p::context.SetHidden(true);
		}

		std::string httpLang; i2p::config::GetOption("http.lang", httpLang);
		i2p::i18n::SetLanguage(httpLang);

		return true;
	}


	bool DaemonWrapper::Start()
	{
		m_runned = true;
	
		i2p::log::Logger().Start();
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Starting NetDB\n");
		i2p::data::netdb.Start();

		// bool upnp; i2p::config::GetOption("upnp.enabled", upnp);
		// if (upnp) {
		// 	d.UPnP = std::unique_ptr<i2p::transport::UPnP>(new i2p::transport::UPnP);
		// 	d.UPnP->Start ();
		// }

		// bool nettime; i2p::config::GetOption("nettime.enabled", nettime);
		// if (nettime)
		// {
		// 	d.m_NTPSync = std::unique_ptr<i2p::util::NTPTimeSync>(new i2p::util::NTPTimeSync);
		// 	d.m_NTPSync->Start ();
		// }

		bool ntcp2; i2p::config::GetOption("ntcp2.enabled", ntcp2);
		bool ssu2; i2p::config::GetOption("ssu2.enabled", ssu2);
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Starting Transports\n");
		if(!ssu2) LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: SSU2 disabled\n");
		if(!ntcp2) LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: NTCP2 disabled\n");

		i2p::transport::transports.Start(ntcp2, ssu2);
		if (i2p::transport::transports.IsBoundSSU2() || i2p::transport::transports.IsBoundNTCP2())
			LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Transports started\n");
		else
		{
			LogPrintLevel(BCLog::Level::Error, BCLog::I2P, "Daemon: Failed to start Transports\n");
			/** shut down netdb right away */
			i2p::transport::transports.Stop();
			i2p::data::netdb.Stop();
			return false;
		}

		// bool http; i2p::config::GetOption("http.enabled", http);
		// if (http) {
		// 	std::string httpAddr; i2p::config::GetOption("http.address", httpAddr);
		// 	uint16_t    httpPort; i2p::config::GetOption("http.port", httpPort);
		// 	LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Starting Webconsole at ", httpAddr, ":", httpPort);
		// 	try
		// 	{
		// 		d.httpServer = std::unique_ptr<i2p::http::HTTPServer>(new i2p::http::HTTPServer(httpAddr, httpPort));
		// 		d.httpServer->Start();
		// 	}
		// 	catch (std::exception& ex)
		// 	{
		// 		LogPrintf (eLogCritical, "Daemon: Failed to start Webconsole: ", ex.what ());
		// 		ThrowFatal ("Unable to start webconsole at ", httpAddr, ":", httpPort, ": ", ex.what ());
		// 	}
		// }

		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Starting Tunnels\n");
		i2p::tunnel::tunnels.Start();

		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Starting Router context\n");
		i2p::context.Start();

		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Starting Client\n");
		i2p::client::context.Start();

		// // I2P Control Protocol
		// bool i2pcontrol; i2p::config::GetOption("i2pcontrol.enabled", i2pcontrol);
		// if (i2pcontrol) {
		// 	std::string i2pcpAddr; i2p::config::GetOption("i2pcontrol.address", i2pcpAddr);
		// 	uint16_t    i2pcpPort; i2p::config::GetOption("i2pcontrol.port",    i2pcpPort);
		// 	LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Starting I2PControl at ", i2pcpAddr, ":", i2pcpPort);
		// 	try
		// 	{
		// 		d.m_I2PControlService = std::unique_ptr<i2p::client::I2PControlService>(new i2p::client::I2PControlService (i2pcpAddr, i2pcpPort));
		// 		d.m_I2PControlService->Start ();
		// 	}
		// 	catch (std::exception& ex)
		// 	{
		// 		LogPrintf(eLogCritical, "Daemon: Failed to start I2PControl: ", ex.what ());
		// 		ThrowFatal ("Unable to start I2PControl service at ", i2pcpAddr, ":", i2pcpPort, ": ", ex.what ());
		// 	}
		// }

		return true;
	}

	bool DaemonWrapper::Stop()
	{
		if (!m_runned)
		{
			m_runned = false;
			return true;
		}

		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Shutting down\n");
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Stopping Client\n");
		i2p::client::context.Stop();
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Stopping Router context\n");
		i2p::context.Stop();
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Stopping Tunnels\n");
		i2p::tunnel::tunnels.Stop();

		// if (d.UPnP)
		// {
		// 	d.UPnP->Stop ();
		// 	d.UPnP = nullptr;
		// }

		// if (d.m_NTPSync)
		// {
		// 	d.m_NTPSync->Stop ();
		// 	d.m_NTPSync = nullptr;
		// }

		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Stopping Transports\n");
		i2p::transport::transports.Stop();
		LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Stopping NetDB\n");
		i2p::data::netdb.Stop();
		// if (d.httpServer) {
		// 	LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Stopping HTTP Server");
		// 	d.httpServer->Stop();
		// 	d.httpServer = nullptr;
		// }
		// if (d.m_I2PControlService)
		// {
		// 	LogPrintLevel(BCLog::Level::Info, BCLog::I2P, "Daemon: Stopping I2PControl");
		// 	d.m_I2PControlService->Stop ();
		// 	d.m_I2PControlService = nullptr;
		// }
		i2p::crypto::TerminateCrypto ();
		i2p::log::Logger().Stop();

		return true;
	}

	std::string DaemonWrapper::GetSAMAddress() {
		std::string samAddr;
		i2p::config::GetOption("sam.address", samAddr);
		return samAddr;
	}

	uint16_t DaemonWrapper::GetSAMPort() {
		uint16_t samPort;
		i2p::config::GetOption("sam.port", samPort);
		return samPort;
	}

}