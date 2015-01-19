#include <tuple>
#include <functional>
#include <map>
#include <iostream>
#include <cmath>
#include <cstdio>

// 手の種類.
enum class enum_hand{
    // 攻撃.
    attack,

    // ガード
    guard,

    // パス.
    pass
};

// 手.
struct hand{
    enum_hand hand_kind = enum_hand::pass;
    int value = 0;
    
    // 一意性確保のための比較用関数.
    bool operator <(hand const &other) const{
        if(hand_kind < other.hand_kind){
            return true;
        }else if(hand_kind > other.hand_kind){
            return false;
        }else{
            return value < other.value;
        }
    }
};

// 駒.
struct man{
    // ヒットポイント初期値.
    static int const hp_max = 10;

    // 攻撃可能回数初期値.
    static int const attack_num_max = 3;
    
    // ガード可能回数初期値.
    static int const guard_max = 2;

    // ヒットポイント.
    int hp = hp_max;

    // 残り攻撃可能回数.
    int attack_num_rest[3];

    // 残りガード可能回数.
    int guard_rest = guard_max;

    man(){
        for(int i = 0; i < sizeof(attack_num_rest) / sizeof(*attack_num_rest); ++i){
            attack_num_rest[i] = attack_num_max;
        }
    }

    // 評価関数.
    // 今のところhpのみで評価.
    int app() const{
        return hp;
    }

    // ターンを回す.
    static void turn(man &red, man &black, hand const &hand_red_side, hand const &hand_black_side){
        if(hand_red_side.hand_kind == enum_hand::attack){
            if(hand_black_side.hand_kind == enum_hand::guard){
                red.hp -= hand_red_side.value;
            }else{
                black.hp -= hand_red_side.value;
            }
            --red.attack_num_rest[hand_red_side.value - 1];
        }else if(hand_red_side.hand_kind == enum_hand::guard){
            --red.guard_rest;
        }
    }
};

struct stage{
    // red = 先手.
    // black = 後手.
    man red, black;

    // このstageで計算される赤の手.
    hand hand_red_side;

    // blackが直前に選んだ手.
    hand hand_black_side;

    // この局面でのredの評価.
    int app_stage_red_side;

    // red側から見た総合評価値を計算する.
    void short_app(){
        app_stage_red_side = red.app() - black.app();
    }
    
    // 取りうる手ごとに状態を保存する.
    std::map<hand, stage> subtree;

    // 状態を再帰的に計算する.
    void compute_subtree(int consider_depth){
        if(consider_depth <= 0){
            short_app();
            return;
        }

        // 行動 = 攻撃.
        for(int i = 0; i < sizeof(red.attack_num_rest) / sizeof(*red.attack_num_rest); ++i){
            if(red.attack_num_rest[i] <= 0){
                continue;
            }

            stage s;
            s.red = black;
            s.black = red;
            hand h;
            h.hand_kind = enum_hand::attack;
            h.value = i + 1;
            s.hand_black_side = h;
            man::turn(s.black, s.red, h, hand_black_side);
            --s.black.attack_num_rest[i];
            subtree[h] = s;
            subtree[h].compute_subtree(consider_depth - 1);
        }

        // 行動 = ガード.
        if(red.guard_rest > 0){
            stage s;
            s.red = black;
            s.black = red;
            --s.black.guard_rest;

            hand h;
            h.hand_kind = enum_hand::guard;
            s.hand_black_side = h;
            subtree[h] = s;
            subtree[h].compute_subtree(consider_depth - 1);
        }

        // 行動 = パス.
        {
            stage s;
            s.red = black;
            s.black = red;

            hand h;
            h.hand_kind = enum_hand::pass;
            s.hand_black_side = h;
            subtree[h] = s;
            subtree[h].compute_subtree(consider_depth - 1);
        }

        // subtreeの手をソートして取るべき手を決定する.
        std::map<int, stage const*, std::less<int>> app_score_to_stage;
        for(auto const &s : subtree){
            app_score_to_stage[s.second.app_stage_red_side] = &s.second;
        }
        app_stage_red_side = app_score_to_stage.begin()->second->black.app() - app_score_to_stage.begin()->second->red.app();
        hand_red_side = app_score_to_stage.begin()->second->hand_black_side;
    }
};

#include <string>

hand get_hand(){
    while(1){
        std::string input;
        std::cin >> input;
        if(input == "attack1" || input == "1"){
            hand h;
            h.hand_kind = enum_hand::attack;
            h.value = 1;
            return h;
        }

        if(input == "attack2" || input == "2"){
            hand h;
            h.hand_kind = enum_hand::attack;
            h.value = 2;
            return h;
        }

        if(input == "attack3" || input == "3"){
            hand h;
            h.hand_kind = enum_hand::attack;
            h.value = 3;
            return h;
        }

        if(input == "guard" || input == "g"){
            hand h;
            h.hand_kind = enum_hand::guard;
            return h;
        }

        if(input == "pass" || input == "p"){
            break;
        }

        std::cout << "不正な入力です." << std::endl;
    }

    hand h;
    h.hand_kind = enum_hand::pass;
    return h;
}

void print_man(man const &p){
    std::cout << "hp = " << (p.hp < 0 ? 0 : p.hp) << ", attack = [";
    for(int i = 0; i < sizeof(p.attack_num_rest) / sizeof(*p.attack_num_rest); ++i){
        if(i != 0){
            std::cout << ", ";
        }
        std::cout << p.attack_num_rest[i];
    }
    std::cout << "], guard = " << p.guard_rest << ".";
}

void game(int lookahead_num){
    lookahead_num = lookahead_num * 2;

    man player, cpu;
    ++cpu.guard_rest;
    while(true){
        std::cout << "player:\t";
        print_man(player);
        std::cout << std::endl;
        
        std::cout << "cpu:\t";
        print_man(cpu);
        std::cout << std::endl;

        if(player.hp <= 0){
            std::cout << "\ncpu win..." << std::endl;
            break;
        }else if(cpu.hp <= 0){
            std::cout << "\nplayer win..." << std::endl;
            break;
        }

        hand hand_player_side, hand_cpu_side;
        
        stage s;
        s.red = cpu;
        s.black = player;
        s.compute_subtree(lookahead_num);
        hand_cpu_side = s.hand_red_side;

        while(true){
            std::cout << "\n>> ";
            hand_player_side = get_hand();
            if(hand_player_side.hand_kind == enum_hand::attack){
                if(player.attack_num_rest[hand_player_side.value - 1] <= 0){
                    std::cout << "攻撃に必要な手数が0です. 再入力してください." << std::endl;
                    continue;
                }
            }else if(hand_player_side.hand_kind == enum_hand::guard){
                if(player.guard_rest <= 0){
                    std::cout << "ガードに必要な手数が0です. 再入力してください." << std::endl;
                    continue;
                }
            }
            break;
        };

        std::cout << "\n";
        std::cout << ">> CPU: ";
        if(hand_cpu_side.hand_kind == enum_hand::attack){
            std::cout << "attack = " << hand_cpu_side.value;
            if(hand_player_side.hand_kind == enum_hand::guard){
                std::cout << "\n\nreferection, to cpu -> " << hand_cpu_side.value << ".";
            }
        }else if(hand_cpu_side.hand_kind == enum_hand::guard){
            std::cout << "guard.";
            if(hand_player_side.hand_kind == enum_hand::attack){
                std::cout << "\n\nreferection, to player -> " << hand_player_side.value << ".";
            }
        }else if(hand_cpu_side.hand_kind == enum_hand::pass){
            std::cout << "pass...";
        }
        std::cout << "\n" << std::endl;

        man::turn(player, cpu, hand_player_side, hand_cpu_side);
        man::turn(cpu, player, hand_cpu_side, hand_player_side);
    }
}

int main(){
    std::cout << ">> ** 遊び方 **\n"
              << ">> - hp     : ゼロになったら負け.\n"
              << ">> - attack : それぞれ, [攻撃力1の残り使用回数, 2, 3]\n"
              << ">> - guard  : ガード残り使用回数\n"
              << ">> [1], [2], [3]キーで攻撃, [g]キーでガード, [p]キーでパスを選択です.\n"
              << ">> 今のところ3手先まで読みます.\n\n";
    game(4);
    std::getchar(), std::getchar();
    return 0;
}

