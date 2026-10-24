// The examples from README.md, so the documentation can't drift from the code.

#include "Event.h"
#include "TestHarness.h"

using namespace BLEvents;

namespace
{
    struct Player
    {
        Event<void, int> onDamaged;
    };

    class HealthBar
    {
    public:
        explicit HealthBar(Player& player) : player(player)
        {
            player.onDamaged += EventCallback(this, &HealthBar::OnDamaged);
        }

        ~HealthBar()
        {
            player.onDamaged -= EventCallback(this, &HealthBar::OnDamaged);
        }

        void OnDamaged(int amount) { shown -= amount; }

        int shown = 100;

    private:
        Player& player;
    };
}

TEST_CASE(QuickStartExample)
{
    Player player;
    {
        HealthBar bar(player);
        player.onDamaged.Invoke(25);
        CHECK(bar.shown == 75);
    }
    CHECK(player.onDamaged.Empty()); // the destructor unsubscribed
}

TEST_CASE(KeepTheCallbackToRemoveALambda)
{
    Event<void, int> onDamaged;
    int hp = 100;
    EventCallback<void, int> onHit = [&hp](int amount) { hp -= amount; };
    onDamaged += onHit;
    onDamaged.Invoke(10);
    onDamaged -= onHit;
    onDamaged.Invoke(10);
    CHECK(hp == 90);
}
