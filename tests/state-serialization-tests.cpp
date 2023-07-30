#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <fstream>

#include "../src/model.h"
#include "../src/model_serialization.h"
#include "../src/app_serialization.h"

using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using InputBinaryArchive = boost::archive::binary_iarchive;
using OutputArchive = boost::archive::text_oarchive;
using OutputBinaryArchive = boost::archive::binary_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

struct SaveLoadFixture {
    std::ofstream out{"../save_file_test", std::ios::binary};
    OutputBinaryArchive output_archive{out};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const model::PointF p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                model::PointF restored_point{};
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Lost Object Serialization") {
    GIVEN("A lost object") {
        const auto obj = [] {
            LostObject obj{LostObject::Id{42}, 20, {2.0, 5.0}};
            return obj;
        }();
        WHEN("object is serialized") {
            {
                serialization::LostObjectRepr repr{obj};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::LostObjectRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(obj.GetId() == restored.GetId());
                CHECK(obj.GetType() == restored.GetType());
                CHECK(obj.GetPosition() == restored.GetPosition());
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        const auto dog = [] {
            Dog dog{Dog::Id{42}, "Pluto"s, 3.0, model::Road::Id{1}};
            dog.SetPos({2.0, 4.0});
            dog.SetPrevPos({2.0, 4.0});
            dog.SetSpeed(model::Direction::EAST);
            dog.SetDirection(model::Direction::EAST);
            dog.SetScore(42);
            dog.GatherLostObject({model::LostObject::Id{3}, 4, {3.0, 5.0}});
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(dog.GetId() == restored.GetId());
                CHECK(dog.GetName() == restored.GetName());
                CHECK(dog.GetPos() == restored.GetPos());
                CHECK(dog.GetSpeed() == restored.GetSpeed());
                CHECK(dog.GetBagContent() == restored.GetBagContent());
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Game Session Serialization") {
    GIVEN("a game session") {
        const auto dog = [] {
            Dog dog{Dog::Id{42}, "Pluto"s, 3.0, model::Road::Id{1}};
            dog.SetPos({2.0, 4.0});
            dog.SetPrevPos({2.0, 4.0});
            dog.SetSpeed(model::Direction::EAST);
            dog.SetDirection(model::Direction::EAST);
            dog.SetScore(42);
            dog.GatherLostObject({model::LostObject::Id{3}, 4, {3.0, 5.0}});
            return dog;
        }();

        const auto obj = [] {
            LostObject obj{LostObject::Id{42}, 20, {2.0, 5.0}};
            return obj;
        }();

        const auto game_session = [dog, obj] {
            GameSessionBase game_session {GameSessionBase::Id{34}, Map::Id{"map3"}};
            game_session.AddNewDog(dog);
            game_session.AddNewObject(obj);
            return game_session;
        }();

        WHEN("game session is serialized") {
            {
                serialization::GameSessionTestRepr repr{game_session};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::GameSessionTestRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(game_session.GetId() == restored.GetId());
                CHECK(game_session.GetMapId() == restored.GetMapId());
                CHECK(game_session.GetDogs() == restored.GetDogs());
                CHECK(game_session.GetLostObjects() == restored.GetLostObjects());
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Player serialization") {
    GIVEN("a player") {
        const auto player = [] {
            app::PlayerBase player{model::Dog::Id{42}, model::GameSession::Id{11}, "11112222333344445555666677778888"};
            return player;
        }();

        WHEN("player is serialized") {
            {
                serialization::PlayerTestRepr repr{player};
                output_archive << repr;
            }

            THEN("he can be deserialized") {
                InputArchive input_archive{strm};
                serialization::PlayerTestRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(player.GetDogId() == restored.GetDogId());
                CHECK(player.GetToken() == restored.GetToken());
            }
        }
    }
}

SCENARIO_METHOD(SaveLoadFixture, "Save & load file") {
    GIVEN("a game session & a player") {
        const auto dog = [] {
            Dog dog{Dog::Id{42}, "Pluto"s, 3.0, model::Road::Id{1}};
            dog.SetPos({2.0, 4.0});
            dog.SetPrevPos({2.0, 4.0});
            dog.SetSpeed(model::Direction::EAST);
            dog.SetDirection(model::Direction::EAST);
            dog.SetScore(42);
            dog.GatherLostObject({model::LostObject::Id{3}, 4, {3.0, 5.0}});
            return dog;
        }();

        const auto obj = [] {
            LostObject obj{LostObject::Id{42}, 20, {2.0, 5.0}};
            return obj;
        }();

        const auto game_session = [dog, obj] {
            GameSessionBase game_session {GameSessionBase::Id{34}, Map::Id{"map3"}};
            game_session.AddNewDog(dog);
            game_session.AddNewObject(obj);
            return game_session;
        }();

        const auto player = [] {
            app::PlayerBase player{model::Dog::Id{42}, model::GameSession::Id{11}, "11112222333344445555666677778888"};
            return player;
        }();

        WHEN("a game session & a player are saved") {
            {
                serialization::GameSessionTestRepr sessionRepr{game_session};
                output_archive << sessionRepr;
                serialization::PlayerTestRepr playerRepr{player};
                output_archive << playerRepr;
                out.close();
            }

            THEN("they can be loaded") {
                std::ifstream in{"../save_file_test", std::ios::binary};
                InputBinaryArchive input_archive{in};
                serialization::GameSessionTestRepr sessionRepr;
                serialization::PlayerTestRepr playerRepr;
                input_archive >> sessionRepr;
                input_archive >> playerRepr;
                const auto restoredSession = sessionRepr.Restore();
                const auto restoredPlayer = playerRepr.Restore();

                CHECK(game_session.GetId() == restoredSession.GetId());
                CHECK(game_session.GetMapId() == restoredSession.GetMapId());
                CHECK(game_session.GetDogs() == restoredSession.GetDogs());
                CHECK(game_session.GetLostObjects() == restoredSession.GetLostObjects());

                CHECK(player.GetDogId() == restoredPlayer.GetDogId());
                CHECK(player.GetToken() == restoredPlayer.GetToken());
            }
        }
    }
}

SCENARIO_METHOD(SaveLoadFixture, "Save & load multiple sessions & players") {
    GIVEN("a game session & a player") {
        const auto dog1 = [] {
            Dog dog{Dog::Id{42}, "Pluto"s, 3.0, model::Road::Id{1}};
            dog.SetPos({2.0, 4.0});
            dog.SetPrevPos({2.0, 4.0});
            dog.SetSpeed(model::Direction::EAST);
            dog.SetDirection(model::Direction::EAST);
            dog.SetScore(42);
            dog.GatherLostObject({model::LostObject::Id{3}, 4, {3.0, 5.0}});
            return dog;
        }();

        const auto dog2 = [] {
            Dog dog{Dog::Id{31}, "Goofy"s, 4.0, model::Road::Id{2}};
            dog.SetPos({3.0, 5.0});
            dog.SetPrevPos({3.0, 5.0});
            dog.SetSpeed(model::Direction::WEST);
            dog.SetDirection(model::Direction::WEST);
            dog.SetScore(69);
            dog.GatherLostObject({model::LostObject::Id{2}, 4, {2.0, 4.0}});
            return dog;
        }();

        const auto obj1 = [] {
            LostObject obj{LostObject::Id{42}, 20, {2.0, 5.0}};
            return obj;
        }();

        const auto obj2 = [] {
            LostObject obj{LostObject::Id{22}, 40, {3.0, 4.0}};
            return obj;
        }();

        const auto game_session1 = [dog1, obj1] {
            GameSessionBase game_session {GameSessionBase::Id{34}, Map::Id{"map3"}};
            game_session.AddNewDog(dog1);
            game_session.AddNewObject(obj1);
            return game_session;
        }();

        const auto game_session2 = [dog2, obj2] {
            GameSessionBase game_session {GameSessionBase::Id{14}, Map::Id{"map2"}};
            game_session.AddNewDog(dog2);
            game_session.AddNewObject(obj2);
            return game_session;
        }();

        const auto player1 = [] {
            app::PlayerBase player{model::Dog::Id{42}, model::GameSession::Id{11}, "11112222333344445555666677778888"};
            return player;
        }();

        const auto player2 = [] {
            app::PlayerBase player{model::Dog::Id{31}, model::GameSession::Id{32}, "11110000333344445555999977778888"};
            return player;
        }();

        WHEN("a game session & a player are saved") {
            {
                std::vector<serialization::GameSessionTestRepr> sessions;
                sessions.emplace_back(game_session1);
                sessions.emplace_back(game_session2);
                output_archive << sessions;
                std::vector<serialization::PlayerTestRepr> players;
                players.emplace_back(player1);
                players.emplace_back(player2);
                output_archive << players;
                out.close();
            }

            THEN("they can be loaded") {
                std::ifstream in{"../save_file_test", std::ios::binary};
                InputBinaryArchive input_archive{in};
                std::vector<serialization::GameSessionTestRepr> sessions;
                std::vector<serialization::PlayerTestRepr> players;
                input_archive >> sessions;
                input_archive >> players;
                const auto restoredSession1 = sessions[0].Restore();
                const auto restoredSession2 = sessions[1].Restore();
                const auto restoredPlayer1 = players[0].Restore();
                const auto restoredPlayer2 = players[1].Restore();

                CHECK(game_session1.GetId() == restoredSession1.GetId());
                CHECK(game_session1.GetMapId() == restoredSession1.GetMapId());
                CHECK(game_session1.GetDogs() == restoredSession1.GetDogs());
                CHECK(game_session1.GetLostObjects() == restoredSession1.GetLostObjects());

                CHECK(player1.GetDogId() == restoredPlayer1.GetDogId());
                CHECK(player1.GetToken() == restoredPlayer1.GetToken());

                CHECK(game_session2.GetId() == restoredSession2.GetId());
                CHECK(game_session2.GetMapId() == restoredSession2.GetMapId());
                CHECK(game_session2.GetDogs() == restoredSession2.GetDogs());
                CHECK(game_session2.GetLostObjects() == restoredSession2.GetLostObjects());

                CHECK(player2.GetDogId() == restoredPlayer2.GetDogId());
                CHECK(player2.GetToken() == restoredPlayer2.GetToken());
            }
        }
    }
}
