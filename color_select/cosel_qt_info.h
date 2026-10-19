// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Handles data storage and conversions for info widget

#ifndef cosel_qt_info_h
#define cosel_qt_info_h

#include <array>
#include <memory>

#include <QString>

#include "cosel_model.h"
#include "cosel_model_hsx.h"
#include "cosel_qt_selected_color.h"
#include "cosel_qt_slider_label.h"
#include "cosel_qt_tools.h"
#include "cosel_space.h"
#include "cosel_vec.h"


namespace cosel {

enum class info_data_type_t : std::int8_t {
    rgb255,
    rgb100,
    hsl,
    hsv,
    hcl,
    lab,
    luv,
    oklab,

    count
};

static constexpr size_t info_num_types = size_t(info_data_type_t::count);
static inline size_t index(info_data_type_t n) {
    return size_t(n);
}

using info_options_t = std::array<bool, info_num_types>;


namespace details {


///////////////////////////////////////////////////////////////////////////////////////////////////
// Class that defines each supported type

class info_type {
public:
    explicit info_type(info_data_type_t type, QString name,
                       const triplet& editrange_low, const triplet& editrange_high,
                       bool in_gamut = true, int precision = 2) :
        type_(type),
        name_(std::move(name)),
        editrange_low_(editrange_low),
        editrange_high_(editrange_high),
        in_gamut_(in_gamut),
        precision_(precision) {}

    virtual ~info_type() = default;

    info_data_type_t type() const {
        return type_;
    }

    size_t index() const {
        return index_;
    }

    void set_index(size_t n) {
        index_ = n;
    }

    const QString& name() const {
        return name_;
    }

    // This does not indicate the current state of the data. It indicates whether the type itself
    // is constrained to the color space gamut (e.g. RGB, HSV, etc..), or can exceed it, (e.g. LAB,
    // LUV, etc).
    bool in_gamut() const {
        return in_gamut_;
    }

    const triplet& editrange_low() const {
        return editrange_low_;
    }

    const triplet& editrange_high() const {
        return editrange_high_;
    }

    int precision() const {
        return precision_;
    }

    bool error() const {
        return error_;
    }

    void set_error(bool yesno) {
        error_ = yesno;
    }

    // Function that converts an XYZ color to type
    virtual triplet from_xyz(const xyz& c) const = 0;
    // Function that converts type to XYZ color
    virtual xyz to_xyz(const triplet& c) const = 0;

protected:
    void set_editrange(const triplet& low, const triplet& high) {
        editrange_low_ = low;
        editrange_high_ = high;
    }

    void set_name(const QString& name) {
        name_ = name;
    }

private:
    info_data_type_t type_;
    QString name_;
    size_t index_ = 0;
    // Edit range minimums.
    triplet editrange_low_;
    // Edit range maximums.
    triplet editrange_high_;
    // Not a CIE color that can be out of bounds of the gamut
    bool in_gamut_ = true;
    // Edit precision
    int precision_ = 2;
    // Error indicator
    bool error_ = false;

};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Class that defines info data for all supported types

class info_data {
public:
    info_data(info_options_t options_) : options(options_) {
        add_info_types();
    }

    size_t count() const {
        return info.size();
    }

    info_type& get(size_t type) {
        assert(type < info.size());
        return *info[type].get();
    }

    info_type& get(info_data_type_t type) {
        return get(size_t(type));
    }

    template<typename FUNC>
    void foreach_item(FUNC f) {
        for(auto& i : info) {
            auto& item = *i.get();
            f(item);
        }
    }

private:
    info_options_t options = { false };
    std::vector<std::unique_ptr<info_type>> info;

    void add_info_types();

    template<typename T>
    void add() {
        T* typ = new T{};
        std::unique_ptr<info_type> ptr(dynamic_cast<info_type*>(typ));
        typ->set_index(info.size());
        info.emplace_back(std::move(ptr));
    }
};

inline void info_data::add_info_types() {

    // RGB //////////////////////////////////////////////////////////////////////////////////////////
    struct rgb_info : public info_type {
        rgb_info(info_data_type_t type, QString name, double range) :
            info_type(type, std::move(name), triplet(), triplet(range, range, range)),
            scale(range, range, range),
            iscale(1/range, 1/range, 1/range) {
        }

        triplet from_xyz(const xyz& c) const override {
            const auto rgb_data = rgb(c).clamped_gamut().to_gamma().data();
            return rgb_data*scale;
        }

        xyz to_xyz(const triplet& c) const override {
            const auto rgb_c = rgb(c*iscale).to_linear();
            return xyz(rgb_c);
        }
    private:
        triplet scale;
        triplet iscale;
    };

    struct rgb255_info : public rgb_info {
        rgb255_info() : rgb_info(info_data_type_t::rgb255, QObject::tr("RGB"), 255) {}
    };

    struct rgb100_info : public rgb_info {
        rgb100_info() : rgb_info(info_data_type_t::rgb100, QObject::tr("RGB"), 100) {}
    };

    if(options[index(info_data_type_t::rgb255)]) {
        add<rgb255_info>();
    }

    if(options[index(info_data_type_t::rgb100)]) {
        add<rgb100_info>();
    }

    // HSL //////////////////////////////////////////////////////////////////////////////////////////
    struct hsl_info : public info_type {
        hsl_info() :
            info_type(info_data_type_t::hsl, QObject::tr("HSL"), triplet(), triplet(360, 100, 100)),
            scale(360, 100, 100),
            iscale(1.0/360, 1.0/100, 1.0/100) {}

        triplet from_xyz(const xyz& c) const override {
            const auto rgb_c = rgb(c).clamped_gamut();
            const auto hsl_c = hsl(rgb_c);
            return hsl_c.data()*scale;
        }

        xyz to_xyz(const triplet& c) const override {
            const auto hsl_c = hsl(c*iscale);
            const auto rgb_c = hsl_c.to_rgb();
            return xyz(rgb_c);
        }
    private:
        triplet scale;
        triplet iscale;
    };

    if(options[index(info_data_type_t::hsl)]) {
        add<hsl_info>();
    }

    // HSV //////////////////////////////////////////////////////////////////////////////////////////
    struct hsv_info : public info_type {
        hsv_info() :
            info_type(info_data_type_t::hsl, QObject::tr("HSV"), triplet(), triplet(360, 100, 100)),
            scale(360, 100, 100),
            iscale(1.0/360, 1.0/100, 1.0/100) {}

        triplet from_xyz(const xyz& c) const override {
            const auto rgb_c = rgb(c).clamped_gamut();
            const auto hsv_c = hsv(rgb_c);
            return hsv_c.data()*scale;
        }

        xyz to_xyz(const triplet& c) const override {
            const auto hsv_c = hsv(c*iscale);
            const auto rgb_c = hsv_c.to_rgb();
            return xyz(rgb_c);
        }
    private:
        triplet scale;
        triplet iscale;
    };

    if(options[index(info_data_type_t::hsv)]) {
        add<hsv_info>();
    }

    // HCL //////////////////////////////////////////////////////////////////////////////////////////
    struct hcl_info : public info_type {
        hcl_info() : info_type(info_data_type_t::hcl, QObject::tr("HCL"),
                                   triplet(), triplet(360, 200, 100), false, 3) {}

        triplet from_xyz(const xyz& c) const override {
            const auto lab_c = lab(c);
            const auto hcl_c = lab::hcl_t(lab_c);
            return hcl_c.data();
        }

        xyz to_xyz(const triplet& c) const override {
            const auto hcl_c = lab::hcl_t(c);
            const auto lab_c = lab(hcl_c);
            return xyz(lab_c);
        }
    };

    if(options[index(info_data_type_t::hcl)]) {
        add<hcl_info>();
    }

    // LAB //////////////////////////////////////////////////////////////////////////////////////////
    struct lab_info : public info_type {
        lab_info() : info_type(info_data_type_t::lab, QObject::tr("LAB"),
                                   triplet{0, -200, -200}, triplet(100, 200, 200), false, 3) {}

        triplet from_xyz(const xyz& c) const override {
            return lab(c).data();
        }

        xyz to_xyz(const triplet& c) const override {
            return xyz(lab(c));
        }
    };

    if(options[index(info_data_type_t::lab)]) {
        add<lab_info>();
    }

    // LUV //////////////////////////////////////////////////////////////////////////////////////////
    struct luv_info : public info_type {
        luv_info() : info_type(info_data_type_t::luv, QObject::tr("LUV"),
                                   triplet{0, -200, -200}, triplet(100, 200, 200), false, 3) {}

        triplet from_xyz(const xyz& c) const override {
            return luv(c).data();
        }

        xyz to_xyz(const triplet& c) const override {
            return xyz(luv(c));
        }
    };

    if(options[index(info_data_type_t::luv)]) {
        add<luv_info>();
    }

    // OKLAB //////////////////////////////////////////////////////////////////////////////////////////
    struct oklab_info : public info_type {
        oklab_info() : info_type(info_data_type_t::oklab, QObject::tr("okLAB"),
                                     triplet{0, 0, 0}, triplet(1, 1, 1), false, 3) {}

        triplet from_xyz(const xyz& c) const override {
            const auto rgb_c = c.to_rgb_fast();
            return oklab(rgb_c).data();
        }

        xyz to_xyz(const triplet& c) const override {
            const auto oklab_c = oklab(c);
            return xyz(oklab_c.to_rgb());
        }
    };

    if(options[index(info_data_type_t::oklab)]) {
        add<oklab_info>();
    }
}


} // namespace details

} // namespace cosel


#endif // cosel_qt_info_h
