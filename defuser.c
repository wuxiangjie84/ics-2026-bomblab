#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t middle = (uint32_t)(challenge >> 16);

    /* Match the 32-bit arithmetic in p1_step0, p1_step1 and p1_step2. */
    uint32_t step0 = (high ^ UINT32_C(0x4421)) + low * UINT32_C(8);
    uint32_t step1 = (high * UINT32_C(3) + middle) ^ UINT32_C(0xf294);
    uint32_t step2 = (step1 + low * UINT32_C(8)) ^ UINT32_C(0x5284);
    values[0] = step2 ^ step0;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;
    values[0] = (uint32_t)((challenge >> 31) & UINT64_C(0xff)) + UINT32_C(0x18);
    for (uint32_t i = 1; i < count; ++i) {
        unsigned shift = 11 + 2 * (i - 1);
        uint32_t part = (uint32_t)((challenge >> shift) & UINT64_C(0x1f));
        values[i] = values[i - 1] * UINT32_C(5) + (part ^ (i + UINT32_C(11)));
    }
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t key = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = (uint32_t)challenge & UINT32_C(7);
    /* Jump-table order: case_0, case_4, case_3, case_1, case_5, case_7, case_6, case_2. */
    switch (values[0]) {
        case 0: values[1] = key * UINT32_C(3) + UINT32_C(0x6655); break;
        case 1: values[1] = key * UINT32_C(3) + UINT32_C(0xb4f5); break;
        case 2: values[1] = key * UINT32_C(5) + UINT32_C(0x4c7e8); break;
        case 3: values[1] = (key ^ UINT32_C(0xd9d4)) + (key >> 1); break;
        case 4: values[1] = key * UINT32_C(5) + UINT32_C(0x32217); break;
        case 5: values[1] = (key ^ UINT32_C(0xd56a)) + (key >> 2); break;
        case 6: values[1] = (key * UINT32_C(4)) ^ UINT32_C(0xd3ce); break;
        case 7: values[1] = (key * UINT32_C(4)) ^ UINT32_C(0x14ca); break;
    }
    return write_answer(out, cap, values, count);
}

static uint32_t phase4_walk(uint32_t target, uint32_t low, uint32_t high, uint32_t salt) {
    uint32_t middle = low + ((high - low) >> 1);
    if (target == middle) return (middle + UINT32_C(0x6f)) ^ salt;
    if (target < middle) {
        uint32_t child = phase4_walk(target, low, middle - 1, salt);
        return (child * UINT32_C(3)) ^ (middle + salt) ^ UINT32_C(0x1a);
    }
    uint32_t child = phase4_walk(target, middle + 1, high, salt);
    return (child * UINT32_C(3)) ^ (middle + salt) ^ UINT32_C(0x08);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t target = (uint32_t)challenge & UINT32_C(0x7f);
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = phase4_walk(target, 0, 127, salt);
    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    /* Each binary node is 16 bytes: next pointer, uint32_t tag, uint32_t weight.
       Store links as indices into the recovered table, not process addresses. */
    static const struct {
        unsigned next;
        uint32_t tag;
        uint32_t weight;
    } nodes[12] = {
        {9,  0x067e, 0x088c}, {4,  0x040e, 0x2f81},
        {0,  0x0886, 0xe1de}, {6,  0x0134, 0x72ac},
        {7,  0x0a55, 0x4bbe}, {1,  0x0b18, 0x14ac},
        {10, 0x0752, 0x7b3c}, {3,  0x036f, 0xd6eb},
        {12, 0x0982, 0xf515}, {5,  0x0546, 0x8284},
        {8,  0x0254, 0x600d}, {2,  0x006d, 0xa214}
    };
    static const unsigned heads[8] = {11, 2, 0, 9, 5, 1, 4, 7};
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    unsigned node = heads[challenge & UINT64_C(7)];
    unsigned length = 1 + (unsigned)((challenge >> 3) & UINT64_C(3));
    uint32_t total = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32) ^ UINT32_C(0x2cb0);
    for (unsigned i = 0; i < length; ++i) {
        if (node >= 12) return 1;
        total = ((total + nodes[node].weight) * UINT32_C(5)) ^ nodes[node].tag;
        values[1] = nodes[node].tag;
        node = nodes[node].next;
    }
    values[0] = total;
    return write_answer(out, cap, values, count);
}

static uint32_t rotate_left(uint32_t value, unsigned shift) {
    return (value << shift) | (value >> (32 - shift));
}

static uint32_t phase6_operation(unsigned index, uint32_t value) {
    /* Indirect-call table: op_3, op_2, op_4, op_5, op_1, op_4, op_0, op_5. */
    switch (index) {
        case 0: return (value >> 5) | (value << 27);
        case 1: return (value + UINT32_C(0xc2bf)) ^ (value >> 1);
        case 2:
        case 5: return rotate_left(value, 5) ^ UINT32_C(0x43a4);
        case 3:
        case 7: return (value ^ UINT32_C(0xe859)) + (value << 7);
        case 4: return ((value << 7) + UINT32_C(0xd083)) ^ (value * UINT32_C(3));
        case 6: return (value ^ (value >> 2)) + UINT32_C(0x6529);
        default: return 0;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    unsigned first = (unsigned)(challenge & UINT64_C(7));
    unsigned second = (unsigned)((challenge >> 3) & UINT64_C(7));
    uint32_t key = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t intermediate = phase6_operation(first, key) ^ UINT32_C(0x6703);
    values[0] = phase6_operation(second, intermediate) + UINT32_C(0x634);
    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t key = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
    uint32_t history = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t first = (uint32_t)context & UINT32_C(3);
    uint32_t mixed = (rotate_left(key ^ history, 3) + UINT32_C(0xe0dc)) ^ UINT32_C(0x7927);
    uint32_t second = mixed ^ (first * UINT32_C(0xe0dc)) ^ (uint32_t)(context >> 16);
    uint32_t third = second ^ (history + first + UINT32_C(0x7927) + (key >> 3));
    /* The copied object has a 32-byte prefix, the four-byte OPEN marker,
       then the three unsigned decimal fields. No newline belongs in out. */
    int size = snprintf(out, cap,
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32 " %" PRIu32 " %" PRIu32,
                        first, second, third);
    return size < 0 || (size_t)size >= cap;
}
