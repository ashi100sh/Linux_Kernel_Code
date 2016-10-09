/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>

extern int module_log_level; /*!< Store current value of logLevel */

#define LOG_LEVEL_NONE        0       /*!< No logging */
#define LOG_LEVEL_CRITICAL    1       /*!< Critical conditions */
#define LOG_LEVEL_ERROR       2       /*!< Error conditions */
#define LOG_LEVEL_WARNING     3       /*!< Warning conditions */
#define LOG_LEVEL_NOTICE      4       /*!< Normal but significant condition */
#define LOG_LEVEL_INFO        5       /*!< Informational */
#define LOG_LEVEL_DEBUG       6       /*!< Debug-level messages */

/*!
 * \brief Set log level by passing logvalue
 */
#define SET_LOG_LEVEL(num)                                                     \
({                                                                             \
        if (num <= LOG_LEVEL_NONE)                                             \
                module_log_level = LOG_LEVEL_NONE;                             \
        else if (num >= LOG_LEVEL_DEBUG)                                       \
                module_log_level= LOG_LEVEL_DEBUG;                             \
        else                                                                   \
                module_log_level = (num);                                      \
})                                                                             \

/*!
 * \brief Get the current log level value
 */
#define GET_LOG_LEVEL()                                                        \
({                                                                             \
        module_log_level;                                                      \
})                                                                             \

/*!
 * \brief Logger message wrapper macro
 */
#define LOG_MSG(level, format, args...)                                        \
({                                                                             \
        if (level <= module_log_level) {                                       \
                if (level == LOG_LEVEL_DEBUG)                                  \
                        printk("MODULE_DEBUG   : "format"\n", ##args);         \
                else if (level == LOG_LEVEL_INFO)                              \
                        printk("MODULE_INFO    : "format"\n", ##args);         \
                else if (level == LOG_LEVEL_NOTICE)                            \
                        printk("MODULE_NOTICE  : "format"\n", ##args);         \
                else if (level == LOG_LEVEL_WARNING)                           \
                        printk("MODULE_WARNING : "format"\n", ##args);         \
                else if (level == LOG_LEVEL_ERROR)                             \
                        printk("MODULE_ERROR   : "format"\n", ##args);         \
                else if (level == LOG_LEVEL_CRITICAL)                          \
                        printk("MODULE_CRITICAL: "format"\n", ##args);         \
        }                                                                      \
})                                                                             \

