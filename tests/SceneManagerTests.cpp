#include "Scene/SceneManager.h"
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

struct Counts {
	int entered = 0;
	int exited = 0;
	int updated = 0;
	int drawn = 0;
	int destroyed = 0;
};

class TestScene final : public IScene {
public:
	explicit TestScene(Counts& counts) : counts_(counts) {
	}

	~TestScene() override {
		++counts_.destroyed;
	}

	void OnEnter() override {
		++counts_.entered;
	}

	void OnExit() noexcept override {
		++counts_.exited;
	}

	void Update() override {
		++counts_.updated;

		if (onUpdate) {
			onUpdate();
		}
	}

	void Draw() override {
		++counts_.drawn;

		if (onDraw) {
			onDraw();
		}
	}

	std::function<void()> onUpdate;
	std::function<void()> onDraw;

private:
	Counts& counts_;
};

}

int main() {
	try {
		Counts first, second, cancelled, last, fromDraw;
		{
			SceneManager manager;
			manager.Update();
			manager.Draw();
			Require(!manager.GetCurrentScene(), "Empty manager acquired a scene");

			auto scene = std::make_unique<TestScene>(first);
			auto* firstScene = scene.get();
			manager.ChangeScene(std::move(scene));
			Require(!manager.GetCurrentScene(), "Scene entered before the safe boundary");
			manager.Update();
			manager.Draw();
			Require(first.entered == 1 && first.updated == 1 && first.drawn == 1,
				"Initial scene did not receive lifecycle calls");

			// Updateから戻るまでは、変更元のthisを破棄してはならない
			firstScene->onUpdate = [&]() {
				manager.ChangeScene(std::make_unique<TestScene>(second));
				Require(first.destroyed == 0 && second.entered == 0,
					"Scene was replaced while its Update was still running");
			};
			manager.Update();
			Require(first.exited == 1 && first.destroyed == 1 && second.entered == 1 &&
				second.updated == 0, "Deferred scene transition failed");
			manager.Draw();
			Require(second.drawn == 1, "New scene was not drawn after transition");

			// 同じフレームで予約し直した場合は、最後の予約だけを使う
			manager.ChangeScene(std::make_unique<TestScene>(cancelled));
			auto lastState = std::make_unique<TestScene>(last);
			auto* lastScene = lastState.get();
			manager.ChangeScene(std::move(lastState));
			Require(cancelled.destroyed == 1 && cancelled.entered == 0,
				"Cancelled pending scene was not released");
			manager.Update();
			Require(second.exited == 1 && second.destroyed == 1 &&
				last.entered == 1 && last.updated == 1, "Replacement request failed");

			bool nullRejected = false;
			try {
				manager.ChangeScene(nullptr);
			}
			catch (const std::invalid_argument&) {
				nullRejected = true;
			}
			Require(nullRejected && manager.GetCurrentScene(), "Null scene changed the current state");

			lastScene->onDraw = [&]() {
				manager.ChangeScene(std::make_unique<TestScene>(fromDraw));
				Require(last.destroyed == 0, "Draw destroyed its own scene");
			};
			manager.Draw();
			Require(fromDraw.entered == 0, "Draw applied a scene change immediately");
			manager.Update();
			Require(last.destroyed == 1 && fromDraw.entered == 1 && fromDraw.updated == 1,
				"Draw's pending scene was not applied on the next update");

			manager.Clear();
			manager.Clear();
			Require(!manager.GetCurrentScene() && fromDraw.exited == 1 &&
				fromDraw.destroyed == 1, "Clear did not release the scene exactly once");
		}

		Counts scoped;
		{
			SceneManager manager;
			manager.ChangeScene(std::make_unique<TestScene>(scoped));
			manager.Update();
		}
		Require(scoped.exited == 1 && scoped.destroyed == 1,
			"Manager destruction did not clean up the current scene");

		std::cout << "PASS: scene lifecycle, deferred transitions, cancelled requests, draw transitions and ownership.\n";
		return 0;
	}
	catch (const std::exception& error) {
		std::cerr << "FAIL: " << error.what() << '\n';
		return 1;
	}
}