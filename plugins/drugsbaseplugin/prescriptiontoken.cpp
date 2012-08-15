/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main developers : Eric MAEKER, <eric.maeker@gmail.com>                *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#include "prescriptiontoken.h"

#include <drugsbaseplugin/constants.h>
#include <drugsbaseplugin/dailyschememodel.h>
#include <drugsbaseplugin/drugsmodel.h>

#include <coreplugin/constants_tokensandsettings.h>

#include <translationutils/constants.h>

using namespace DrugsDB;
using namespace Trans::ConstantTranslations;

DrugsDB::DrugsModel *PrescriptionToken::_model = 0;
int PrescriptionToken::_row = 0;

PrescriptionToken::PrescriptionToken(const QString &name, const int ref) :
    Core::IToken(name),
    _ref(ref),
    _isRepeatedDailyScheme(false),
    _isDistributedDailyScheme(false),
    _isMeal(false)
{
    _isDistributedDailyScheme = (name==Core::Constants::TOKEN_PRESC_DISTRIB_DAILYSCHEME);
    _isRepeatedDailyScheme = (name==Core::Constants::TOKEN_PRESC_REPEATED_DAILYSCHEME);
    _isMeal = (ref==DrugsDB::Constants::Prescription::MealTimeSchemeIndex);
}

PrescriptionToken::~PrescriptionToken() {}

QVariant PrescriptionToken::testValue() const
{
    return "TESTINGVALUE";
}

QVariant PrescriptionToken::value() const
{
    using namespace DrugsDB::Constants;
    if (_isRepeatedDailyScheme) {
        DrugsDB::DailySchemeModel day;
        day.setSerializedContent(_model->data(_model->index(_row, Prescription::SerializedDailyScheme)).toString());
        return day.humanReadableRepeatedDailyScheme();
    } else if (_isDistributedDailyScheme) {
        DrugsDB::DailySchemeModel day;
        day.setSerializedContent(_model->data(_model->index(_row, Prescription::SerializedDailyScheme)).toString());
        return day.humanReadableDistributedDailyScheme();
    } else if (_isMeal) {
        const QVariant &v = _model->data(_model->index(_row, _ref));
        if (!v.isValid() || v.isNull())
            return QVariant();
        return mealTime(v.toInt());
    } else if (_ref==Prescription::IntakesIntervalSchemeIndex) {
        const QVariant &v = _model->data(_model->index(_row, _ref));
        if (!v.isValid() || v.isNull())
            return QVariant();
        return period(v.toInt());
    } else if (_ref==Prescription::IntakesIntervalOfTime) {
        const QVariant &v = _model->data(_model->index(_row, _ref));
        if (!v.isValid() || v.isNull())
            return QVariant();
        if (v.toInt() <= 0)
            return QVariant();
        return v;

    }
    return _model->data(_model->index(_row, _ref));
}
