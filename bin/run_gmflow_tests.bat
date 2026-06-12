@echo off
setlocal

set CL=C:\AppPortable\clang+llvm-19.1.7-x86_64-pc-windows-msvc\bin\clang++.exe
set ROOT=C:\_GLT_\Qt Prj\game_lib
cd /d "%ROOT%"

set GD= ^
  gmDispatch/Dispatcher.cpp ^
  gmDispatch/DispatcherFactory.cpp ^
  gmDispatch/dispatchers/SyncDispatcher.cpp ^
  gmDispatch/dispatchers/AsyncDispatcher.cpp ^
  gmDispatch/routers/SyncRouter.cpp ^
  gmDispatch/routers/PatternRouter.cpp ^
  gmDispatch/channels/EventBusChannel.cpp ^
  gmDispatch/channels/StdoutChannel.cpp ^
  gmDispatch/channels/FileChannel.cpp ^
  gmDispatch/channels/IpSocketChannel.cpp ^
  gmDispatch/serializers/JsonSerializer.cpp ^
  gmDispatch/bridges/LogDispatchBridge.cpp

set GF_BASE= ^
  gmFlow/core/Result.cpp ^
  gmFlow/core/GameContext.cpp ^
  gmFlow/actors/ActorRegistry.cpp ^
  gmFlow/actions/ActionQueue.cpp ^
  gmFlow/actions/ActionWindow.cpp ^
  gmFlow/actions/StepBasedAction.cpp ^
  gmFlow/flow/Turn.cpp ^
  gmFlow/flow/Round.cpp ^
  gmFlow/flow/SequentialFlowController.cpp ^
  gmFlow/events/EventBus.cpp ^
  gmFlow/session/GameSession.cpp ^
  gmFlow/campaign/Campaign.cpp

echo.
echo === BUILD test_action_queue ===
"%CL%" -std=c++17 -I. -IgmDispatch ^
  gmFlow/core/Result.cpp ^
  gmFlow/actions/ActionQueue.cpp ^
  gmFlow/tests/test_action_queue.cpp ^
  -o test_action_queue.exe
if %ERRORLEVEL% NEQ 0 ( echo BUILD FAILED & goto :next1 )
echo Build OK. Running...
test_action_queue.exe
:next1

echo.
echo === BUILD test_action_window ===
"%CL%" -std=c++17 -I. -IgmDispatch ^
  gmFlow/core/Result.cpp ^
  gmFlow/core/GameContext.cpp ^
  gmFlow/actors/ActorRegistry.cpp ^
  gmFlow/actions/ActionQueue.cpp ^
  gmFlow/actions/ActionWindow.cpp ^
  gmFlow/actions/StepBasedAction.cpp ^
  gmFlow/flow/Turn.cpp ^
  gmFlow/flow/Round.cpp ^
  gmFlow/flow/SequentialFlowController.cpp ^
  gmFlow/events/EventBus.cpp ^
  gmFlow/session/GameSession.cpp ^
  gmFlow/campaign/Campaign.cpp ^
  gmDispatch/Dispatcher.cpp ^
  gmDispatch/DispatcherFactory.cpp ^
  gmDispatch/dispatchers/SyncDispatcher.cpp ^
  gmDispatch/dispatchers/AsyncDispatcher.cpp ^
  gmDispatch/routers/SyncRouter.cpp ^
  gmDispatch/routers/PatternRouter.cpp ^
  gmDispatch/channels/EventBusChannel.cpp ^
  gmDispatch/channels/StdoutChannel.cpp ^
  gmDispatch/channels/FileChannel.cpp ^
  gmDispatch/serializers/JsonSerializer.cpp ^
  gmFlow/tests/test_action_window.cpp ^
  -o test_action_window.exe
if %ERRORLEVEL% NEQ 0 ( echo BUILD FAILED & goto :next2 )
echo Build OK. Running...
test_action_window.exe
:next2

echo.
echo === BUILD test_flow_sequential ===
"%CL%" -std=c++17 -I. -IgmDispatch ^
  gmFlow/core/Result.cpp ^
  gmFlow/core/GameContext.cpp ^
  gmFlow/actors/ActorRegistry.cpp ^
  gmFlow/actions/ActionQueue.cpp ^
  gmFlow/actions/ActionWindow.cpp ^
  gmFlow/actions/StepBasedAction.cpp ^
  gmFlow/flow/Turn.cpp ^
  gmFlow/flow/Round.cpp ^
  gmFlow/flow/SequentialFlowController.cpp ^
  gmFlow/events/EventBus.cpp ^
  gmFlow/session/GameSession.cpp ^
  gmFlow/campaign/Campaign.cpp ^
  gmDispatch/Dispatcher.cpp ^
  gmDispatch/DispatcherFactory.cpp ^
  gmDispatch/dispatchers/SyncDispatcher.cpp ^
  gmDispatch/dispatchers/AsyncDispatcher.cpp ^
  gmDispatch/routers/SyncRouter.cpp ^
  gmDispatch/routers/PatternRouter.cpp ^
  gmDispatch/channels/EventBusChannel.cpp ^
  gmDispatch/channels/StdoutChannel.cpp ^
  gmDispatch/channels/FileChannel.cpp ^
  gmDispatch/serializers/JsonSerializer.cpp ^
  gmFlow/tests/test_flow_sequential.cpp ^
  -o test_flow_sequential.exe
if %ERRORLEVEL% NEQ 0 ( echo BUILD FAILED & goto :next3 )
echo Build OK. Running...
test_flow_sequential.exe
:next3

echo.
echo === BUILD test_flow_campaign ===
"%CL%" -std=c++17 -I. -IgmDispatch ^
  gmFlow/campaign/Campaign.cpp ^
  gmFlow/tests/test_flow_campaign.cpp ^
  -o test_flow_campaign.exe
if %ERRORLEVEL% NEQ 0 ( echo BUILD FAILED & goto :done )
echo Build OK. Running...
test_flow_campaign.exe
:done

echo.
echo === All tests complete ===
endlocal
