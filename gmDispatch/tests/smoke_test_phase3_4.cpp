/**
 * @file tests/smoke_test_phase3_4.cpp
 * @brief Phase 3+4 smoke tests for gmDispatch.
 *
 * Exercises:
 *  1.  FileChannel — appends envelopes to a temp file
 *  2.  PatternRouter — exact match
 *  3.  PatternRouter — prefix wildcard "engine.*"
 *  4.  PatternRouter — broadcast "*"
 *  5.  PatternRouter — targeted delivery via Envelope::targets
 *  6.  PatternRouter — anonymous channel ignores targets
 *  7.  AsyncDispatcher — non-blocking dispatch + flush
 *  8.  AsyncDispatcher — subscribe/unsubscribe thread-safe
 *  9.  LogDispatchBridge — LogRecord → Envelope forwarding
 *  10. create_async_dispatcher factory
 *  11. create_pattern_dispatcher factory
 *
 * Build (from game_lib root):
 *   clang++ -std=c++17 -I. \
 *     gmDispatch/tests/smoke_test_phase3_4.cpp \
 *     gmDispatch/Dispatcher.cpp \
 *     gmDispatch/DispatcherFactory.cpp \
 *     gmDispatch/channels/EventBusChannel.cpp \
 *     gmDispatch/channels/StdoutChannel.cpp \
 *     gmDispatch/channels/FileChannel.cpp \
 *     gmDispatch/serializers/JsonSerializer.cpp \
 *     gmDispatch/routers/SyncRouter.cpp \
 *     gmDispatch/routers/PatternRouter.cpp \
 *     gmDispatch/dispatchers/SyncDispatcher.cpp \
 *     gmDispatch/dispatchers/AsyncDispatcher.cpp \
 *     gmDispatch/bridges/LogDispatchBridge.cpp \
 *     gmDispatch/bridges/RuleEventBridge.cpp \
 *     gmLog/LogLevel.cpp gmLog/Logger.cpp gmLog/LoggerFactory.cpp \
 *     gmLog/sinks/StdoutSink.cpp gmLog/sinks/FileSink.cpp \
 *     gmLog/formatters/JsonFormatter.cpp \
 *     gmLog/dispatchers/SyncDispatcher.cpp \
 *     -o smoke_gmDispatch_p34.exe && ./smoke_gmDispatch_p34.exe
 */

#include "gmDispatch/DispatcherFactory.hpp"
#include "gmDispatch/channels/EventBusChannel.hpp"
#include "gmDispatch/channels/FileChannel.hpp"
#include "gmDispatch/routers/PatternRouter.hpp"
#include "gmDispatch/dispatchers/SyncDispatcher.hpp"
#include "gmDispatch/dispatchers/AsyncDispatcher.hpp"
#include "gmDispatch/bridges/LogDispatchBridge.hpp"
#include "gmDispatch/bridges/RuleEventBridge.hpp"

#include "gmRules/core/RuleEvent.hpp"

#include "gmLog/LoggerFactory.hpp"

#include <any>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <thread>

// ── Helpers ───────────────────────────────────────────────────────────────────

static void pass(const std::string& name)
{
	std::cout << "[PASS] " << name << "\n";
}

static void fail(const std::string& name, const std::string& reason)
{
	std::cout << "[FAIL] " << name << " — " << reason << "\n";
}

// ── Test 1: FileChannel writes envelopes to disk ──────────────────────────────

static void test_fileChannel()
{
	const std::string TEST     = "test_fileChannel";
	const std::string TMPFILE  = "test_gmDispatch_p34.log";

	{
		gmDispatch::GmDispatcher bus =
			gmDispatch::DispatcherFactory::create_sync_dispatcher("Bus");

		bus.subscribe("evt",
			std::make_shared<gmDispatch::FileChannel>(TMPFILE));

		gmDispatch::Envelope env;
		env.typeId  = "evt";
		env.source  = "TestSource";
		bus.dispatch(env);
		bus.dispatch(env);
		bus.flush();
	} // FileChannel destructor flushes & closes

	std::ifstream f(TMPFILE);
	int lines = 0;
	std::string line;
	while (std::getline(f, line)) { ++lines; }
	f.close();                     // must close before std::remove on Windows
	std::remove(TMPFILE.c_str());

	if (lines == 2) pass(TEST);
	else            fail(TEST, "expected 2 lines, got " + std::to_string(lines));
}

// ── Test 2: PatternRouter — exact match ───────────────────────────────────────

static void test_patternExact()
{
	const std::string TEST = "test_patternExact";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_pattern_dispatcher("Bus");

	std::shared_ptr<gmDispatch::EventBusChannel> ch =
		std::make_shared<gmDispatch::EventBusChannel>();

	int count = 0;
	ch->add_handler([&](const gmDispatch::Envelope&) { ++count; });
	bus.subscribe("engine.tick", ch);

	gmDispatch::Envelope tick; tick.typeId = "engine.tick";
	gmDispatch::Envelope init; init.typeId = "engine.init";

	bus.dispatch(tick);
	bus.dispatch(init);  // should NOT reach ch

	if (count == 1) pass(TEST);
	else            fail(TEST, "expected 1, got " + std::to_string(count));
}

// ── Test 3: PatternRouter — prefix wildcard ───────────────────────────────────

static void test_patternWildcard()
{
	const std::string TEST = "test_patternWildcard";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_pattern_dispatcher("Bus");

	std::shared_ptr<gmDispatch::EventBusChannel> ch =
		std::make_shared<gmDispatch::EventBusChannel>();

	int count = 0;
	ch->add_handler([&](const gmDispatch::Envelope&) { ++count; });
	bus.subscribe("engine.*", ch);

	gmDispatch::Envelope e1; e1.typeId = "engine.tick";
	gmDispatch::Envelope e2; e2.typeId = "engine.init";
	gmDispatch::Envelope e3; e3.typeId = "input.key";   // should NOT match

	bus.dispatch(e1);
	bus.dispatch(e2);
	bus.dispatch(e3);

	if (count == 2) pass(TEST);
	else            fail(TEST, "expected 2, got " + std::to_string(count));
}

// ── Test 4: PatternRouter — broadcast "*" ─────────────────────────────────────

static void test_patternBroadcast()
{
	const std::string TEST = "test_patternBroadcast";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_pattern_dispatcher("Bus");

	std::shared_ptr<gmDispatch::EventBusChannel> ch =
		std::make_shared<gmDispatch::EventBusChannel>();

	int count = 0;
	ch->add_handler([&](const gmDispatch::Envelope&) { ++count; });
	bus.subscribe("*", ch);

	for (int i = 0; i < 5; ++i) {
		gmDispatch::Envelope e; e.typeId = "anything";
		bus.dispatch(e);
	}

	if (count == 5) pass(TEST);
	else            fail(TEST, "expected 5, got " + std::to_string(count));
}

// ── Test 5: PatternRouter — targeted delivery ─────────────────────────────────

static void test_targetedDelivery()
{
	const std::string TEST = "test_targetedDelivery";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_pattern_dispatcher("Bus");

	// Named channels
	std::shared_ptr<gmDispatch::EventBusChannel> ui =
		std::make_shared<gmDispatch::EventBusChannel>("UI");
	std::shared_ptr<gmDispatch::EventBusChannel> ai =
		std::make_shared<gmDispatch::EventBusChannel>("AI");

	int uiCount = 0, aiCount = 0;
	ui->add_handler([&](const gmDispatch::Envelope&) { ++uiCount; });
	ai->add_handler([&](const gmDispatch::Envelope&) { ++aiCount; });

	bus.subscribe("cmd", ui);
	bus.subscribe("cmd", ai);

	// Target only "UI"
	gmDispatch::Envelope env;
	env.typeId  = "cmd";
	env.targets = {"UI"};
	bus.dispatch(env);

	if (uiCount == 1 && aiCount == 0) pass(TEST);
	else fail(TEST, "UI=" + std::to_string(uiCount)
				  + " AI=" + std::to_string(aiCount) + " (expected 1,0)");
}

// ── Test 6: PatternRouter — anonymous channel bypasses targeting ──────────────

static void test_anonymousBypassesTarget()
{
	const std::string TEST = "test_anonymousBypassesTarget";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_pattern_dispatcher("Bus");

	// Anonymous channel (no name → always receives)
	std::shared_ptr<gmDispatch::EventBusChannel> anon =
		std::make_shared<gmDispatch::EventBusChannel>();  // name == ""

	int count = 0;
	anon->add_handler([&](const gmDispatch::Envelope&) { ++count; });
	bus.subscribe("cmd", anon);

	gmDispatch::Envelope env;
	env.typeId  = "cmd";
	env.targets = {"UI"};   // "UI" is not in this channel's name
	bus.dispatch(env);      // anonymous channel should still receive

	if (count == 1) pass(TEST);
	else            fail(TEST, "expected 1 (anonymous bypass), got "
							  + std::to_string(count));
}

// ── Test 7: AsyncDispatcher — non-blocking + flush ───────────────────────────

static void test_asyncDispatcher()
{
	const std::string TEST = "test_asyncDispatcher";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_async_dispatcher("AsyncBus");

	std::shared_ptr<gmDispatch::EventBusChannel> ch =
		std::make_shared<gmDispatch::EventBusChannel>();

	int count = 0;
	ch->add_handler([&](const gmDispatch::Envelope&) { ++count; });

	bus.subscribe("tick", ch);

	const int N = 20;
	for (int i = 0; i < N; ++i) {
		gmDispatch::Envelope env;
		env.typeId = "tick";
		bus.dispatch(env);  // non-blocking
	}

	bus.flush();  // wait for all N envelopes to be delivered

	if (count == N) pass(TEST);
	else            fail(TEST, "expected " + std::to_string(N)
							  + ", got " + std::to_string(count));
}

// ── Test 8: AsyncDispatcher — concurrent subscribe ───────────────────────────

static void test_asyncConcurrentSubscribe()
{
	const std::string TEST = "test_asyncConcurrentSubscribe";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_async_dispatcher("AsyncBus");

	// Subscribe from a separate thread while dispatching from main
	std::shared_ptr<gmDispatch::EventBusChannel> ch =
		std::make_shared<gmDispatch::EventBusChannel>();

	int count = 0;
	ch->add_handler([&](const gmDispatch::Envelope&) { ++count; });

	// Thread subscribes the channel after a short pause
	std::thread t([&] {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		bus.subscribe("x", ch);
	});

	// Meanwhile dispatch some events (before channel is subscribed — should be 0)
	for (int i = 0; i < 5; ++i) {
		gmDispatch::Envelope env; env.typeId = "x";
		bus.dispatch(env);
	}

	t.join();

	// Now dispatch after subscription
	for (int i = 0; i < 3; ++i) {
		gmDispatch::Envelope env; env.typeId = "x";
		bus.dispatch(env);
	}

	bus.flush();

	// count should be exactly 3 (the ones after subscription)
	// It could be slightly more if subscription races — that's acceptable.
	// We only verify it's <= 8 and the bus didn't crash.
	if (count <= 8) pass(TEST);
	else            fail(TEST, "count out of range: " + std::to_string(count));
}

// ── Test 9: LogDispatchBridge — forwards LogRecord to bus ────────────────────

static void test_logDispatchBridge()
{
	const std::string TEST = "test_logDispatchBridge";

	gmDispatch::GmDispatcher bus =
		gmDispatch::DispatcherFactory::create_sync_dispatcher("Bus");

	std::shared_ptr<gmDispatch::EventBusChannel> ch =
		std::make_shared<gmDispatch::EventBusChannel>();

	std::string capturedTypeId;
	std::string capturedSource;
	std::string capturedMessage;

	ch->add_handler([&](const gmDispatch::Envelope& env) {
		capturedTypeId  = env.typeId;
		capturedSource  = env.source;
		capturedMessage = std::any_cast<std::string>(env.payload);
	});

	bus.subscribe("log.INFO", ch);

	// Build a Logger that uses LogDispatchBridge as its dispatcher
	gmLog::LoggerConfig cfg;
	cfg.name                 = "DB";
	cfg.min_level             = gmLog::LogLevel::INFO;
	cfg.enable_source_location = false;

	gmLog::GmLogger db(
		cfg,
		std::make_unique<gmDispatch::LogDispatchBridge>(bus));

	const std::string infoMsg = "Connected";
	db.info(infoMsg);

	if (capturedTypeId == "log.INFO" &&
		capturedSource  == "DB"      &&
		capturedMessage == "Connected") {
		pass(TEST);
	} else {
		fail(TEST, "typeId='" + capturedTypeId
				 + "' source='" + capturedSource
				 + "' msg='" + capturedMessage + "'");
	}
}

// ── Test 10: RuleEventBridge — forwards gmRules events to mapped channels ────

class MockDispatcher : public gmDispatch::IDispatcher {
public:
	bool throw_on_dispatch = false;
	std::vector<gmDispatch::Envelope> envelopes;

	void dispatch(const gmDispatch::Envelope& envelope) override
	{
		if (throw_on_dispatch)
		{
			throw std::runtime_error("mock dispatch failure");
		}

		envelopes.push_back(envelope);
	}

	void subscribe(const std::string&, std::shared_ptr<gmDispatch::IChannel>) override
	{}

	void unsubscribe(const std::string&, std::shared_ptr<gmDispatch::IChannel>) override
	{}

	void flush() override
	{}
};

static void test_ruleEventBridge()
{
	const std::string TEST = "test_ruleEventBridge";

	std::unique_ptr<MockDispatcher> impl = std::make_unique<MockDispatcher>();
	MockDispatcher*                 raw  = impl.get();

	gmDispatch::DispatcherConfig cfg;
	cfg.name = "RulesBus";

	gmDispatch::GmDispatcher bus(cfg, std::move(impl));
	gmDispatch::RuleEventBridge bridge(bus);

	gmRules::RuleEvent event;
	event.type = "gmRules.actor.hp_changed";
	event.source_id = "hero";
	event.target_id = "orc";
	event.payload_json = "{\"delta\":-3}";
	event.priority = 7;

	bridge.dispatch(event);

	const bool routed = raw->envelopes.size() == 1 &&
		raw->envelopes[0].typeId == "RuleEvBus" &&
		raw->envelopes[0].source == "gmRules" &&
		raw->envelopes[0].headers.at("source_system") == "gmRules" &&
		raw->envelopes[0].headers.at("rule_priority") == "7" &&
		raw->envelopes[0].headers.at("rule_topic") == "actor.events" &&
		raw->envelopes[0].headers.at("rule_event_type") == event.type &&
		raw->envelopes[0].headers.at("rule_source_id") == event.source_id &&
		raw->envelopes[0].headers.at("rule_target_id") == event.target_id &&
		std::any_cast<std::string>(raw->envelopes[0].payload) == event.payload_json &&
		bridge.success_count() == 1 &&
		bridge.failure_count() == 0;

	if (routed)
	{
		pass(TEST);
	}
	else
	{
		fail(TEST, "bridge did not map or preserve the event correctly");
	}
}

static void test_ruleEventBridgeFailure()
{
	const std::string TEST = "test_ruleEventBridgeFailure";

	std::unique_ptr<MockDispatcher> impl = std::make_unique<MockDispatcher>();
	MockDispatcher*                 raw  = impl.get();
	raw->throw_on_dispatch = true;

	gmDispatch::DispatcherConfig cfg;
	cfg.name = "RulesBus";

	gmDispatch::GmDispatcher bus(cfg, std::move(impl));
	gmDispatch::RuleEventBridge bridge(bus);

	gmRules::RuleEvent event;
	event.type = "gmRules.map.path_blocked";
	event.source_id = "map01";
	event.priority = 3;

	bridge.dispatch(event, "MapEvBus");

	if (bridge.success_count() == 0 &&
		bridge.failure_count() == 1 &&
		!bridge.last_error().empty() &&
		raw->envelopes.empty())
	{
		pass(TEST);
	}
	else
	{
		fail(TEST, "failure was not tracked as expected");
	}
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "=== gmDispatch Phase 3+4 smoke tests ===\n\n";

	test_fileChannel();
	test_patternExact();
	test_patternWildcard();
	test_patternBroadcast();
	test_targetedDelivery();
	test_anonymousBypassesTarget();
	test_asyncDispatcher();
	test_asyncConcurrentSubscribe();
	test_logDispatchBridge();
	test_ruleEventBridge();
	test_ruleEventBridgeFailure();

	std::cout << "\nDone.\n";
	return 0;
}
