
#include <cstdint>
extern "C" {
/********************************** */
/*WB */
/********************************** */
int E_IF_encode(void *st, int16_t mode, int16_t *speech, uint8_t *serial,
                int16_t dtx);
void *E_IF_init(void);
void E_IF_exit(void *state);
enum {
  kAmrGoodFrame = 0,
  kAmrBadFrame = 1,
  kAmrLostFrame = 2,
  kAmrNoFrame = 3
};
void D_IF_decode(void *st, uint8_t *bits, int16_t *synth, int32_t bfi);
void *D_IF_init(void);
void D_IF_exit(void *state);
}
