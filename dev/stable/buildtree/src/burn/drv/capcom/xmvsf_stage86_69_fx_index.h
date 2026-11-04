#ifndef XMVSF_STAGE86_69_FX_INDEX_H
#define XMVSF_STAGE86_69_FX_INDEX_H
// XM86.69 P1_FX_INDEX -- deterministic, conservative and player-independent.
// Source: user captured xm8666_sprite_inventory.tsv (30,000 observations)
// P2 reference: priority 6 palettes 16-23; P1 reference palettes 0-15.
// Exact tile+palette pairs only, NEVER 256-tile family promotion.
// Critical caveat: grade values are previous renderer classifications, not
// ground truth. Only P1 pairs also observed as P2 FX tiles are promoted;
// shared fighter tile/palette pairs are explicitly blocked.
#include <cstdint>
#include <algorithm>
#include <iterator>
namespace XM8669 {
enum Classification : unsigned { Unseen=0u, ConfirmedFX=1u, MixedFX=2u, BodyConflict=3u };
static constexpr std::uint32_t kConfirmed[] = {
    4430509u, 4572526u, 4572590u, 4572622u, 4572686u, 4572782u, 4573230u, 4573614u,
    4573774u, 4574318u, 4574734u, 4576078u, 4576686u, 4576910u, 4577325u, 4577421u,
    4577453u, 4577837u, 4578061u, 4578317u, 4578797u, 4579245u, 4605389u, 4607373u,
    4608013u, 4608365u, 4608973u, 4610061u, 4610541u, 4610765u, 4610925u, 4611021u,
    4611149u, 4611405u, 4611565u, 4611597u, 4612109u, 4612269u, 4612333u, 4612589u,
    4612749u, 4612941u, 4613037u, 4613357u, 4613645u, 4614157u, 4614349u, 4614797u,
    4615309u, 4615629u, 4618189u, 4618253u, 4618381u, 4618573u, 4618925u, 4619149u,
    4629997u, 4641421u, 4641773u, 4641805u, 4642381u, 4643053u, 4643341u, 4643853u,
    4644045u, 4644173u, 4644301u, 4644557u, 4644621u, 4645357u, 4645389u, 4645549u,
    4645677u, 4645837u, 4646061u, 4646669u, 4647181u, 4647693u, 4647949u, 6343502u,
    6359022u, 6398446u, 6415148u, 6415150u, 6415151u, 6811054u, 6839054u, 7547662u,
    7556238u, 7556366u, 7556462u, 7556558u, 7556622u, 7556878u, 7557166u, 7557646u,
    7557902u, 7558414u, 7558542u, 7558926u, 7559182u, 7561422u, 7576654u, 7576750u,
    7576846u, 7577006u, 7577166u, 7577582u, 7577614u, 7577806u, 7577902u, 7578062u,
    7578190u, 7578318u, 7578702u, 7578894u, 7579470u, 7579982u, 7580174u, 7580558u,
    7580686u, 7590350u, 7590414u, 7590670u, 7591310u, 7591502u, 7591598u, 7591662u,
    7591950u, 7592046u, 7592750u, 7593390u, 7593550u, 7594222u, 7594286u, 7594446u,
    7594510u, 7594830u, 7595022u, 7595150u, 7660014u, 7662254u, 7703917u,
};
static constexpr std::uint32_t kMixed[] = {
    4266093u, 6309870u, 6318287u, 6593294u, 7084206u,
};
static constexpr std::uint32_t kBodyConflict[] = {
    4195215u, 4211663u, 4238541u, 4238543u, 4266092u, 4266095u, 4286159u, 4286351u,
    4286575u, 4286863u, 4286927u, 4293999u, 4299727u, 4300655u, 4579853u, 6296204u,
    6296207u, 6303533u, 6309868u, 6309871u, 6318284u, 6593292u, 7084202u, 7436845u,
    7495086u, 7569711u, 7581198u, 7581583u, 7581647u, 7581807u, 7582319u, 7583439u,
    7583855u, 7636015u, 7636687u, 7637007u, 7637199u, 7651630u, 7659532u, 7659535u,
    7659788u, 7659791u, 7659852u, 7659855u, 7660044u, 7660047u, 7660236u, 7660239u,
    7660364u, 7660367u, 7660748u, 7660751u, 7661068u, 7661580u, 7662318u, 7662414u,
    7662510u, 7662574u, 7662606u, 7662734u, 7663118u, 7663630u, 7663758u, 7667820u,
    7667823u, 7668044u, 7668047u, 7668236u, 7668239u, 7668556u, 7668559u, 7668748u,
    7668876u, 7669388u, 7669580u, 7670604u, 7670766u, 7670894u, 7671054u, 7671214u,
    7671406u, 7671534u, 7671694u, 7671758u, 7672142u, 7673358u, 7673824u, 7700492u,
    7700556u, 7700780u, 7700876u, 7700940u, 7700972u, 7701004u, 7701580u, 7701583u,
    7701740u, 7701743u, 7701836u, 7701839u, 7701868u, 7701871u, 7702028u, 7702031u,
    7702477u, 7702541u, 7702829u, 7702893u, 7703021u, 7703565u, 7704269u, 7704685u,
    7705101u, 7705421u, 7705741u, 7705901u, 7706413u, 7706541u, 7706637u, 7707149u,
    7707373u, 7707629u, 7709837u, 7709901u, 7709997u, 7711245u, 7711341u, 7711469u,
};
static inline bool Contains(const std::uint32_t* p, std::size_t n, std::uint32_t key){
    return std::binary_search(p,p+n,key);
}
static inline Classification Classify(unsigned priority,unsigned palette,unsigned tile){
    if(priority!=6u || palette>=16u || tile>0x07FFFFFFu)return Unseen;
    const std::uint32_t key=(std::uint32_t(tile)<<5)|palette;
    if(Contains(kBodyConflict,sizeof(kBodyConflict)/sizeof(kBodyConflict[0]),key))return BodyConflict;
    if(Contains(kConfirmed,sizeof(kConfirmed)/sizeof(kConfirmed[0]),key))return ConfirmedFX;
    if(Contains(kMixed,sizeof(kMixed)/sizeof(kMixed[0]),key))return MixedFX;
    return Unseen;
}
}
#endif
