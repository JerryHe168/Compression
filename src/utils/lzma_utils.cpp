#include "compression/utils/lzma_utils.h"
#include <cstring>
#include <algorithm>

#ifdef HAVE_LZMA
#include <lzma.h>
#endif

namespace Compression {
namespace Utils {

namespace {

const uint64_t crc64Table[256] = {
    0x0000000000000000ULL, 0x7ad870c830358979ULL,
    0xf5b0e190606b12f2ULL, 0x8f689158505e9b8bULL,
    0xc038e5739841b68fULL, 0xbae095bba8743ff6ULL,
    0x358804e3f82aa47dULL, 0x4f50742bc81f2d04ULL,
    0xab28ecb46814fe75ULL, 0xd1f09c7c5821770cULL,
    0x5e980d24087fec87ULL, 0x24407dec384a65feULL,
    0x6b1009c7f05548faULL, 0x11c8790fc060c183ULL,
    0x9ea0e857903e5a08ULL, 0xe478989fa00bd371ULL,
    0x7d08ff3b88be6f81ULL, 0x07d08ff3b88be6f8ULL,
    0x88b81eabe8d57d73ULL, 0xf2606e63d8e0f40aULL,
    0xbd301a4810ffd90eULL, 0xc7e86a8020ca5077ULL,
    0x4880fbd87094cbfcULL, 0x32588b1040a14285ULL,
    0xd620138fe0aa91f4ULL, 0xacf86347d09f188dULL,
    0x2390f21f80c18306ULL, 0x594882d7b0f40a7fULL,
    0x1618f6fc78eb277bULL, 0x6cc0863448deae02ULL,
    0xe3a8176c18803589ULL, 0x997067a428b5bc00ULL,
    0xfa11fe77117cdf02ULL, 0x80c98ebf2149567bULL,
    0x0fa11fe77117cdf0ULL, 0x75796f2f41224489ULL,
    0x3a291b04893d698dULL, 0x40f16bccb908e0f4ULL,
    0xcf99fa94e9567b7fULL, 0xb5418a5cd963f206ULL,
    0x513912c379682177ULL, 0x2be1620b495da80eULL,
    0xa489f35319033385ULL, 0xde51839b2936baffULL,
    0x9101f7b0e12997f8ULL, 0xebd98778d11c1e81ULL,
    0x64b116208142850aULL, 0x1e6966e8b1770c73ULL,
    0x8719014c99c2b083ULL, 0xfdc17184a9f739faULL,
    0x72a9e0dcf9a9a271ULL, 0x08719014c99c2b08ULL,
    0x4721e43f0183060cULL, 0x3df994f731b68f75ULL,
    0xb29105af61e814feULL, 0xc849756751dd9d87ULL,
    0x2c31edf8f1d64ef6ULL, 0x56e99d30c1e3c78fULL,
    0xd9810c6891bd5c04ULL, 0xa3597ca0a188d57dULL,
    0xec09088b6997f879ULL, 0x96d1784359a27100ULL,
    0x19b9e91b09fcea8bULL, 0x636199d339c963f2ULL,
    0xdfad8cf45d355de5ULL, 0xa575fc3c6d00d49cULL,
    0x2a1d6d643d5e4f17ULL, 0x50c51dac0d6bc66eULL,
    0x1f956987c574eb6aULL, 0x654d194ff5416213ULL,
    0xea258817a51ff998ULL, 0x90fdf8df952a70e1ULL,
    0x748560403521a390ULL, 0x0e5d108805142ae9ULL,
    0x813581d0554ab162ULL, 0xfbedf118657f381bULL,
    0xb4bd8533ad60151fULL, 0xce65f5fba9559c66ULL,
    0x410d64a3cd0b07edULL, 0x3bd5146bfd3e8e94ULL,
    0xa2a573cfd58b3264ULL, 0xd87d0307e5bebb1dULL,
    0x5715925fb5e02096ULL, 0x2dcde29785d5a9efULL,
    0x629d96bc4dc884ebULL, 0x1845e6747dfd0d92ULL,
    0x972d772c2da39619ULL, 0xedf507e41d961f60ULL,
    0x098d9f7bbdecce11ULL, 0x7355efb38ed94768ULL,
    0xfc3d7eebddd7dce3ULL, 0x86e50e23ede2559aULL,
    0xc9b57a0825ad789eULL, 0xb36d0ac01598f1e7ULL,
    0x3c059b9845c66a6cULL, 0x46ddeb5075f3e315ULL,
    0x25bc26894c4982e7ULL, 0x5f6456417c7c0b9eULL,
    0xd00cc7192c229015ULL, 0xaad4b7d11c17196cULL,
    0xe584c3fad4083468ULL, 0x9f5cb332e43dbd11ULL,
    0x1034226ab463269aULL, 0x6aec52a28456afe3ULL,
    0x8e94ca3d245d7c92ULL, 0xf44cbaf51468f5ebULL,
    0x7b242bad44366e60ULL, 0x01fc5b657403e719ULL,
    0x4eac2f4ebc1cc01dULL, 0x34745f868c294964ULL,
    0xbb1ccdea40766edULL, 0xc1c4be169432efd4ULL,
    0x581cdfb9287306efULL, 0x22c4af7118468f96ULL,
    0x9c8d3f2e909954ddULL, 0xe6554fe6a0acdd04ULL,
    0x09208de2678d8025ULL, 0x73f8fd2a574da4d2ULL,
    0xcc10d6a953a97668ULL, 0xb6c8a661639cff11ULL,
    0x39a0373933c2649aULL, 0x437847f103f7edf3ULL,
    0x67342a1d4b4d881aULL, 0x1dec5ad57b780163ULL,
    0x9284cb8d2b269ae8ULL, 0xe85cbb451b131391ULL,
    0xa70ccf6ed30c3e95ULL, 0xddd4bfa6e339b7ecULL,
    0x52bc2efeb3672c67ULL, 0x28645e368352a51eULL,
    0xcc3cc6a92359766eULL, 0xb6e4b661136cff17ULL,
    0x398c27394332649cULL, 0x435457f17307edf5ULL,
    0x0c0423dabbdc20e1ULL, 0x76dc53128becac56ULL,
    0xf9b4c24adbb73213ULL, 0x836cb282ebb2ab6aULL,
    0xa15353d8e9fa48e6ULL, 0xdb8b2310d9cfc19fULL,
    0x54e3b24889915a14ULL, 0x2e3bc280b9a4d36dULL,
    0x616bb6ab71bbfe69ULL, 0x1bb3c663418e7710ULL,
    0x94db573b11d0ec9bULL, 0xee0327f321e565e2ULL,
    0x0a7bbf6c81eeb693ULL, 0x70a3cfa4b1db3feaULL,
    0xffcb5efced85a461ULL, 0x85132e34dd80b976ULL,
    0xca435a1f19af001cULL, 0xb09b2ad7299a8965ULL,
    0x3ff3bb8f79c412eeULL, 0x452bcb4749f19b97ULL,
    0x524bdee361442767ULL, 0x2893ae2b5171ae1eULL,
    0xa7fb3f73012f3595ULL, 0xdd234fbb311abcecULL,
    0x92733b90f90591e8ULL, 0xe8ab4b58c9301891ULL,
    0x67c3da00996e831aULL, 0x1d1baac8a95b0a63ULL,
    0xf96332570950d912ULL, 0x83bb429f3965506bULL,
    0x0cd3d3c7693bcbe0ULL, 0x760ba30f590e4299ULL,
    0x395bd72491116f9dULL, 0x4383a7eca124e6e4ULL,
    0xcceb36b4f17a7d6fULL, 0xb633467cc14ff416ULL
};

}

uint64_t LZMAUtils::getInitialCRC64()
{
    return 0xFFFFFFFFFFFFFFFFULL;
}

uint64_t LZMAUtils::calculateCRC64(uint64_t crc, const std::vector<uint8_t>& data)
{
    for (uint8_t byte : data) {
        crc = crc64Table[static_cast<uint8_t>(crc ^ byte)] ^ (crc >> 8);
    }
    return crc;
}

uint64_t LZMAUtils::calculateCRC64(const std::vector<uint8_t>& data)
{
    uint64_t crc = getInitialCRC64();
    crc = calculateCRC64(crc, data);
    return crc ^ 0xFFFFFFFFFFFFFFFFULL;
}

void LZMAUtils::writeVarInt(std::vector<uint8_t>& output, uint64_t value)
{
    for (int i = 0; i < 9; ++i) {
        uint8_t b = static_cast<uint8_t>(value & 0x7F);
        value >>= 7;
        if (value != 0) {
            b |= 0x80;
        }
        output.push_back(b);
        if (value == 0) {
            break;
        }
    }
}

uint64_t LZMAUtils::readVarInt(const std::vector<uint8_t>& input, size_t& offset)
{
    return readVarInt(input.data(), input.size(), offset);
}

uint64_t LZMAUtils::readVarInt(const uint8_t* input, size_t maxSize, size_t& offset)
{
    uint64_t value = 0;
    int shift = 0;
    while (offset < maxSize && shift < 63) {
        uint8_t b = input[offset++];
        value |= static_cast<uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            return value;
        }
        shift += 7;
    }
    return value;
}

size_t LZMAUtils::getVarIntSize(uint64_t value)
{
    size_t size = 0;
    do {
        value >>= 7;
        size++;
    } while (value != 0);
    return size;
}

bool LZMAUtils::isLzmaAvailable()
{
#ifdef HAVE_LZMA
    return true;
#else
    return false;
#endif
}

bool LZMAUtils::compress(const std::vector<uint8_t>& input,
                          std::vector<uint8_t>& output,
                          const LZMAConfig& config)
{
#ifdef HAVE_LZMA
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_options_lzma opt_lzma2;

    if (lzma_lzma_preset(&opt_lzma2, LZMA_PRESET_DEFAULT)) {
        return false;
    }

    opt_lzma2.dict_size = config.dictSize;
    opt_lzma2.lc = config.lc;
    opt_lzma2.lp = config.lp;
    opt_lzma2.pb = config.pb;

    lzma_filter filters[2];
    filters[0].id = LZMA_FILTER_LZMA2;
    filters[0].options = &opt_lzma2;
    filters[1].id = LZMA_VLI_UNKNOWN;

    if (lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC64) != LZMA_OK) {
        return false;
    }

    strm.next_in = const_cast<uint8_t*>(input.data());
    strm.avail_in = input.size();

    output.resize(input.size() + 1024);
    strm.next_out = output.data();
    strm.avail_out = output.size();

    lzma_ret ret = lzma_code(&strm, LZMA_FINISH);

    if (ret != LZMA_STREAM_END) {
        lzma_end(&strm);
        return false;
    }

    output.resize(strm.total_out);
    lzma_end(&strm);
    return true;
#else
    output = input;
    return true;
#endif
}

bool LZMAUtils::decompress(const std::vector<uint8_t>& input,
                            std::vector<uint8_t>& output,
                            size_t expectedSize)
{
#ifdef HAVE_LZMA
    lzma_stream strm = LZMA_STREAM_INIT;

    if (lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED) != LZMA_OK) {
        return false;
    }

    strm.next_in = const_cast<uint8_t*>(input.data());
    strm.avail_in = input.size();

    output.resize(expectedSize + 1024);
    strm.next_out = output.data();
    strm.avail_out = output.size();

    lzma_ret ret = lzma_code(&strm, LZMA_FINISH);

    if (ret != LZMA_STREAM_END && ret != LZMA_OK) {
        lzma_end(&strm);
        return false;
    }

    output.resize(strm.total_out);
    lzma_end(&strm);
    return true;
#else
    output = input;
    return true;
#endif
}

bool LZMAUtils::compressWithProps(const std::vector<uint8_t>& input,
                                    std::vector<uint8_t>& output,
                                    std::vector<uint8_t>& props,
                                    const LZMAConfig& config)
{
#ifdef HAVE_LZMA
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_options_lzma opt_lzma;

    opt_lzma.dict_size = config.dictSize;
    opt_lzma.lc = config.lc;
    opt_lzma.lp = config.lp;
    opt_lzma.pb = config.pb;
    opt_lzma.fb = config.fb;
    opt_lzma.algo = config.algo;
    opt_lzma.bt_mode = 1;
    opt_lzma.num_fast_bytes = config.fb;
    opt_lzma.match_finder = LZMA_MF_HC4;
    opt_lzma.mc = 32;

    lzma_filter filters[2];
    filters[0].id = LZMA_FILTER_LZMA1;
    filters[0].options = &opt_lzma;
    filters[1].id = LZMA_VLI_UNKNOWN;

    if (lzma_raw_encoder(&strm, filters) != LZMA_OK) {
        return false;
    }

    strm.next_in = const_cast<uint8_t*>(input.data());
    strm.avail_in = input.size();

    output.resize(input.size() + 1024);
    strm.next_out = output.data();
    strm.avail_out = output.size();

    lzma_ret ret = lzma_code(&strm, LZMA_FINISH);

    if (ret != LZMA_STREAM_END) {
        lzma_end(&strm);
        return false;
    }

    output.resize(strm.total_out);
    lzma_end(&strm);

    props.resize(5);
    props[0] = static_cast<uint8_t>((config.pb * 5 + config.lp) * 9 + config.lc);
    
    uint32_t dictSize = config.dictSize;
    props[1] = static_cast<uint8_t>(dictSize & 0xFF);
    props[2] = static_cast<uint8_t>((dictSize >> 8) & 0xFF);
    props[3] = static_cast<uint8_t>((dictSize >> 16) & 0xFF);
    props[4] = static_cast<uint8_t>((dictSize >> 24) & 0xFF);

    return true;
#else
    output = input;
    props.resize(5, 0);
    return true;
#endif
}

bool LZMAUtils::decompressWithProps(const std::vector<uint8_t>& input,
                                     const std::vector<uint8_t>& props,
                                     std::vector<uint8_t>& output,
                                     size_t expectedSize)
{
#ifdef HAVE_LZMA
    if (props.size() < 5) {
        return false;
    }

    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_options_lzma opt_lzma;

    uint8_t lc_lp_pb = props[0];
    opt_lzma.lc = lc_lp_pb % 9;
    uint8_t remainder = lc_lp_pb / 9;
    opt_lzma.lp = remainder % 5;
    opt_lzma.pb = remainder / 5;

    uint32_t dictSize = 0;
    dictSize |= static_cast<uint32_t>(props[1]);
    dictSize |= static_cast<uint32_t>(props[2]) << 8;
    dictSize |= static_cast<uint32_t>(props[3]) << 16;
    dictSize |= static_cast<uint32_t>(props[4]) << 24;
    opt_lzma.dict_size = dictSize;

    lzma_filter filters[2];
    filters[0].id = LZMA_FILTER_LZMA1;
    filters[0].options = &opt_lzma;
    filters[1].id = LZMA_VLI_UNKNOWN;

    if (lzma_raw_decoder(&strm, filters) != LZMA_OK) {
        return false;
    }

    strm.next_in = const_cast<uint8_t*>(input.data());
    strm.avail_in = input.size();

    output.resize(expectedSize);
    strm.next_out = output.data();
    strm.avail_out = output.size();

    lzma_ret ret = lzma_code(&strm, LZMA_FINISH);

    if (ret != LZMA_STREAM_END && ret != LZMA_OK) {
        lzma_end(&strm);
        return false;
    }

    output.resize(strm.total_out);
    lzma_end(&strm);
    return true;
#else
    output = input;
    return true;
#endif
}

}
}
