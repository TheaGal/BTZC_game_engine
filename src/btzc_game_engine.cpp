#include "btzc_game_engine.h"

#define APP_NAME "Untitled Game"


#ifdef NDEBUG
#define DEBUG_BUILD_WARNING_MSG
#else
#define DEBUG_BUILD_WARNING_MSG " - ！！ご注意：実行中のこのアプリは DEBUG BUILD である！！"
#endif // NDEBUG


char const k_application_title_str[]{ APP_NAME " - " BTZC_GAME_ENGINE_OS_NAME
                                               " - " BTZC_GAME_ENGINE_DEV_STAGE
                                               " - " BTZC_GAME_ENGINE_VERSION
                                               " - compiled on " __DATE__
                                               " - commit " GIT_COMMIT_HASH
                                               DEBUG_BUILD_WARNING_MSG };
