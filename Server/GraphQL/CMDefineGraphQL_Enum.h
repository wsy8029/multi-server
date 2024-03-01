#ifndef _CM_DEFINE_GRAPH_QL_ENUM_H_
#define _CM_DEFINE_GRAPH_QL_ENUM_H_

namespace NestoQL
{
    enum GraphQLType
    {
        QUERY = 0,
        MUTATION
    };

    enum GraphQLResult
    {
        GQR_CURL_ERROR = 0,
        GQR_SUCCESS = 1,
        GQR_ERROR = 2
    };

    enum EGraphQLMethodType
    {
        EGQLMT_SignIn = 0,
        EGQLMT_MyProfile,
        EGQLMT_Count
    };

    inline const char* ToString(EGraphQLMethodType v)
    {
        if (v == EGQLMT_SignIn)        return "signIn";
        else if (v == EGQLMT_MyProfile)    return "myProfile";
    }

    enum ProductAvatarType
    {
        FACE_SHAPE,
        SKIN_COLOR,
        FACE_STICKER,
        HAIR,
        TOP,
        BOTTOM,
        ONEPIECE,
        SHOE,
        ACCESSORY,
        EYEBROW,
        EYE,
        NOSE,
        MOUTH
    };

    inline const char* ToString(ProductAvatarType v)
    {
        if (v == FACE_SHAPE)        return "FACE_SHAPE";
        else if (v == SKIN_COLOR)    return "SKIN_COLOR";
        else if (v == FACE_STICKER)    return "FACE_STICKER";
        else if (v == HAIR)    return "HAIR";
        else if (v == TOP)    return "TOP";
        else if (v == BOTTOM)    return "BOTTOM";
        else if (v == ONEPIECE)    return "ONEPIECE";
        else if (v == SHOE)    return "SHOE";
        else if (v == ACCESSORY)    return "ACCESSORY";
        else if (v == EYEBROW)    return "EYEBROW";
        else if (v == EYE)    return "EYE";
        else if (v == NOSE)    return "NOSE";
        else if (v == MOUTH)    return "MOUTH";
    }

    inline const ProductAvatarType ConvertProductAvatarType(const char* v)
    {
        if (strcmp(v, "FACE_SHAPE") == 0) return FACE_SHAPE;
        else if (strcmp(v, "SKIN_COLOR") == 0) return SKIN_COLOR;
        else if (strcmp(v, "FACE_STICKER") == 0) return FACE_STICKER;
        else if (strcmp(v, "HAIR") == 0) return HAIR;
        else if (strcmp(v, "TOP") == 0) return TOP;
        else if (strcmp(v, "BOTTOM") == 0) return BOTTOM;
        else if (strcmp(v, "ONEPIECE") == 0) return ONEPIECE;
        else if (strcmp(v, "SHOE") == 0) return SHOE;
        else if (strcmp(v, "ACCESSORY") == 0) return ACCESSORY;
        else if (strcmp(v, "EYEBROW") == 0) return EYEBROW;
        else if (strcmp(v, "EYE") == 0) return EYE;
        else if (strcmp(v, "NOSE") == 0) return NOSE;
        else if (strcmp(v, "MOUTH") == 0) return MOUTH;
    }

    enum CharacterStatusType
    {
        DEFAULT,
        HAPPY,
        SAD,
        DANCE1,
        DANCE2,
        YES,
        NO,
        TIRED,
        HELLO
    };

    inline const char* ToString(CharacterStatusType v)
    {
        if (v == DEFAULT)        return "DEFAULT";
        else if (v == HAPPY)    return "HAPPY";
        else if (v == SAD)    return "SAD";
        else if (v == DANCE1)    return "DANCE1";
        else if (v == DANCE2)    return "DANCE2";
        else if (v == YES)    return "YES";
        else if (v == NO)    return "NO";
        else if (v == TIRED)    return "TIRED";
        else if (v == HELLO)    return "HELLO";
    }

    inline const CharacterStatusType ConvertCharacterStatusType(const char* v)
    {
        if (strcmp(v, "DEFAULT") == 0) return DEFAULT;
        else if (strcmp(v, "HAPPY") == 0) return HAPPY;
        else if (strcmp(v, "SAD") == 0) return SAD;
        else if (strcmp(v, "DANCE1") == 0) return DANCE1;
        else if (strcmp(v, "DANCE2") == 0) return DANCE2;
        else if (strcmp(v, "YES") == 0) return YES;
        else if (strcmp(v, "NO") == 0) return NO;
        else if (strcmp(v, "TIRED") == 0) return TIRED;
        else if (strcmp(v, "HELLO") == 0) return HELLO;
    }

}

#endif
