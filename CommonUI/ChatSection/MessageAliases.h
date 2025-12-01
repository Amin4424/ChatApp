#ifndef MESSAGEALIASES_H
#define MESSAGEALIASES_H

// Type aliases for backward compatibility
#include "TextMessage.h"
#include "FileMessage.h"
#include "AudioMessage.h"
#include "MessageDirection.h"

using TextMessageItem = TextMessage;
using FileMessageItem = FileMessage;
using VoiceMessageItem = AudioMessage;

#endif // MESSAGEALIASES_H
