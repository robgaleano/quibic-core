using namespace QPI;

struct HM252
{
    // Future expansion state
};

struct HM25 : public ContractBase
{
public:
    static const uint64 MAX_PLAYERS = 100;
    static const uint64 TOTAL_SHARES_PER_PLAYER = 1000000; // 1M shares per player
    static const uint64 MIN_INVESTMENT = 100; // Minimum investment amount in QUs

    struct Player {
        id playerId;
        char name[32];
        char team[32];
        uint64 targetAmount;
        uint64 currentAmount;
        uint64 sharesAvailable;
        uint64 sharePrice;
        bool fundingComplete;
        bool active;
    };

    struct Investment {
        id investorId;
        id playerId;
        uint64 shares;
        uint64 amount;
    };

    // Input/Output structures for procedures
    struct CreatePlayer_input {
        char name[32];
        char team[32];
        uint64 targetAmount;
        uint64 sharePrice;
    };
    struct CreatePlayer_output {
        id playerId;
        bool success;
    };

    struct InvestInPlayer_input {
        id playerId;
        uint64 amount;
    };
    struct InvestInPlayer_output {
        uint64 sharesReceived;
        bool success;
    };

    struct GetPlayerInfo_input {
        id playerId;
    };
    struct GetPlayerInfo_output {
        Player player;
        bool exists;
    };

    struct GetInvestorShares_input {
        id investorId;
        id playerId;
    };
    struct GetInvestorShares_output {
        uint64 shares;
        uint64 amount;
    };

private:
    Player players[MAX_PLAYERS];
    uint64 playerCount;
    
    // Mapping of investor to their investments (simplified implementation)
    Investment investments[1000];
    uint64 investmentCount;

    PUBLIC_PROCEDURE(CreatePlayer)
        if (state.playerCount >= MAX_PLAYERS) {
            output.success = false;
            return;
        }

        Player& newPlayer = state.players[state.playerCount];
        // Generate unique player ID using the player count as a simple incrementing identifier
        newPlayer.playerId = state.playerCount + 1;
        
        // Copy name and team with bounds checking
        for (uint64 i = 0; i < 31 && input.name[i] != 0; i++) {
            newPlayer.name[i] = input.name[i];
        }
        newPlayer.name[31] = 0;
        
        for (uint64 i = 0; i < 31 && input.team[i] != 0; i++) {
            newPlayer.team[i] = input.team[i];
        }
        newPlayer.team[31] = 0;
        
        newPlayer.targetAmount = input.targetAmount;
        newPlayer.currentAmount = 0;
        newPlayer.sharesAvailable = TOTAL_SHARES_PER_PLAYER;
        newPlayer.sharePrice = input.sharePrice;
        newPlayer.fundingComplete = false;
        newPlayer.active = true;

        output.playerId = newPlayer.playerId;
        output.success = true;
        state.playerCount++;
    _

    PUBLIC_PROCEDURE(InvestInPlayer)
        if (input.amount < MIN_INVESTMENT || qpi.invocationReward() < input.amount) {
            output.success = false;
            return;
        }

        Player* player = nullptr;
        for (uint64 i = 0; i < state.playerCount; i++) {
            if (state.players[i].playerId == input.playerId && state.players[i].active) {
                player = &state.players[i];
                break;
            }
        }

        if (player == nullptr || player->fundingComplete) {
            output.success = false;
            return;
        }

        uint64 sharesToBuy = input.amount / player->sharePrice;
        if (sharesToBuy == 0 || sharesToBuy > player->sharesAvailable) {
            output.success = false;
            return;
        }

        // Record investment
        Investment& inv = state.investments[state.investmentCount++];
        inv.investorId = qpi.invocator();
        inv.playerId = input.playerId;
        inv.shares = sharesToBuy;
        inv.amount = input.amount;

        // Update player state
        player->currentAmount += input.amount;
        player->sharesAvailable -= sharesToBuy;
        
        if (player->currentAmount >= player->targetAmount) {
            player->fundingComplete = true;
        }

        output.sharesReceived = sharesToBuy;
        output.success = true;
    _

    PUBLIC_FUNCTION(GetPlayerInfo)
        for (uint64 i = 0; i < state.playerCount; i++) {
            if (state.players[i].playerId == input.playerId) {
                output.player = state.players[i];
                output.exists = true;
                return;
            }
        }
        output.exists = false;
    _

    PUBLIC_FUNCTION(GetInvestorShares)
        output.shares = 0;
        output.amount = 0;
        
        for (uint64 i = 0; i < state.investmentCount; i++) {
            if (state.investments[i].investorId == input.investorId && 
                state.investments[i].playerId == input.playerId) {
                output.shares += state.investments[i].shares;
                output.amount += state.investments[i].amount;
            }
        }
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(CreatePlayer, 1);
        REGISTER_USER_PROCEDURE(InvestInPlayer, 2);
        REGISTER_USER_FUNCTION(GetPlayerInfo, 1);
        REGISTER_USER_FUNCTION(GetInvestorShares, 2);
    _

    INITIALIZE
        state.playerCount = 0;
        state.investmentCount = 0;
    _
};
