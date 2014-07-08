/*
 *  drivers/cpufreq/cpufreq_april.c
 *
 *  Copyright (C)  2013 Johnlay Park (jonglae.park@samsung.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#ifdef CONFIG_ARM_EXYNOS5410_CPUFREQ
#include <linux/fb.h>
#include <linux/pm_qos.h>
#include <mach/cpufreq.h>
#endif

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define MIN_SAMPLE_RATE					(10000)
#define MIN_FREQUENCY_UP_THRESHOLD		(10)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)

#define MIN_SAMPLE_RATE_MULT			(1)
#define MAX_SAMPLE_RATE_MULT			(100)

#define DEF_TRANSCOST_UP_THRESHOLD		(95)
#define DEF_TRANSCOST_DOWN_DIFF			(3)
#define DEF_TRANSDELAY_UP_DELAY			(1)
#define DEF_TRANSDELAY_DOWN_DELAY		(1)

enum {SCALE_NONE, SCALE_UP, SCALE_DOWN, SCALE_BOTH};

#ifdef CONFIG_ARM_EXYNOS5410_CPUFREQ
#define LOWPOWER_SAMPLING_RATE			(100000);

#define MIN_FREQUENCY_UP_STEP_LEVEL		(500000)
#define MAX_FREQUENCY_UP_STEP_LEVEL		(1800000)

/*
	For convenience, Data should be ordered by Frequency ascendantly.
*/
/*
	Stopover frequency when level up or level down.
	{stopover frequency}, {stopover case}
*/
static unsigned int default_trans_steps[] = {
	600000, 	SCALE_UP,
	800000,		SCALE_DOWN,
	1200000,	SCALE_UP,
};

/*
	Transition cost (delay, threshold @ each dvfs level
	({frequency}), {up threshold}, {down differential}
*/
static unsigned int default_trans_costs[] = {
	60,	20,			// ~ 600MHz : Threshold = 60% / -20%
	800000, 90,	3,	// 800MHz ~ : Threshold = 90% / - 3%
};


static unsigned int lowpower_trans_costs[] = {
	95, 10,
	800000, 99, 3,
};

/*
	Transition delays (delay, threshold @ each dvfs level
	({frequency}), {rate mult when level up}, {rate mult when level down}
*/
static unsigned int default_trans_delays[] = {
	1,1,
};

static unsigned int lowpower_trans_delays[] = {
	4,1,
};

#else
static unsigned int default_trans_steps[] = {};
static unsigned int default_trans_costs[] = {
	DEF_TRANSCOST_UP_THRESHOLD, DEF_TRANSCOST_DOWN_DIFF
};
static unsigned int default_trans_delays[] = {
	DEF_TRANSDELAY_UP_DELAY, DEF_TRANSDELAY_DOWN_DELAY
};
#endif // CONFIG_ARM_EXYNOS5410_CPUFREQ
static spinlock_t	trans_lock;

static unsigned int *trans_steps = default_trans_steps;
static unsigned int *trans_costs = default_trans_costs;
static unsigned int *trans_delays = default_trans_delays;
static unsigned int ntrans_steps = ARRAY_SIZE(default_trans_steps);
static unsigned int ntrans_costs = ARRAY_SIZE(default_trans_costs);
static unsigned int ntrans_delays = ARRAY_SIZE(default_trans_delays);

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

static void do_dbs_timer(struct work_struct *work);
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_APRIL
static
#endif
struct cpufreq_governor cpufreq_gov_april = {
       .name                   = "april",
       .governor               = cpufreq_governor_dbs,
       .max_transition_latency = TRANSITION_LATENCY_LIMIT,
       .owner                  = THIS_MODULE,
};

/* Sampling types */
enum {DBS_LONG_SAMPLE, DBS_SHORT_SAMPLE};

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_iowait;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;

	unsigned int longterm_load_freq;
	unsigned int longterm_count;

	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	struct cpufreq_frequency_table *freq_table;
	int cpu;
	unsigned int sample_type:1;	// DBS_SHORT_SAMPLE
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
	unsigned int sampling_rate;		// unit sampling rate
	unsigned int up_threshold;
	unsigned int down_differential;
	unsigned int ignore_nice;
	unsigned int io_is_busy;
	unsigned int up_step_level;
	unsigned int down_step_level;
	unsigned int up_ratemult;		// multiply of sampling rate
	unsigned int down_ratemult;

	u64			 longterm_jiffies;
	u64			 shortterm_jiffies;
	u64			 validate_timestamp;
	unsigned int shortterm_direction;
} dbs_tuners_ins = {
	.up_threshold = DEF_TRANSCOST_UP_THRESHOLD,
	.down_differential = DEF_TRANSCOST_DOWN_DIFF,
	.up_step_level = UINT_MAX,
	.down_step_level = 0,
	.up_ratemult = DEF_TRANSDELAY_UP_DELAY,
	.down_ratemult = DEF_TRANSDELAY_DOWN_DELAY,
	.ignore_nice = 0,
};

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
/*
 * Increase this value if cpu load is less than base load of hotplug
 * out condition.
 */
static bool lcd_is_on;

#define ENABLE_HOTPLUG_OUT_H

/* This means permitted consecutive boost level */
#define BOOST_LV_CNT   			20

#define HOTPLUG_OUT_LOAD		10

#define HOTPLUG_OUT_CNT_H		6
#define HOTPLUG_OUT_CNT_L		5

#define HOTPLUG_TRANS_H_BTM		(800000)
#define HOTPLUG_TRANS_H			1600000
#define HOTPLUG_TRANS_H_CPUS		2
#define HOTPLUG_TRANS_L			250000
#define HOTPLUG_TRANS_L_CPUS		1

#define UP_THRESHOLD_FB_BLANK		(90)

static struct cpumask out_cpus;
static struct cpumask to_be_out_cpus;
static struct work_struct hotplug_work;
static bool hotplug_out;
static unsigned int consecutive_boost_level;

static DEFINE_PER_CPU(unsigned int, hotplug_out_cnt_h);
static int hotplug_out_cnt_l;

static DEFINE_MUTEX(hotplug_mutex);

static void do_hotplug(struct work_struct *work)
{
	unsigned int cpu, ret;

	mutex_lock(&hotplug_mutex);
	if (hotplug_out) {
		for_each_cpu(cpu, &to_be_out_cpus) {
			if (cpu == 0)
				continue;

			ret = cpu_down(cpu);
			if (ret) {
				pr_debug("%s: CPU%d down fail: %d\n",
					__func__, cpu, ret);
				continue;
			} else {
				cpumask_set_cpu(cpu, &out_cpus);
			}
		}
		cpumask_clear(&to_be_out_cpus);
	} else {
		for_each_cpu(cpu, &out_cpus) {
			if (cpu == 0)
				continue;

			ret = cpu_up(cpu);
			if (ret) {
				pr_debug("%s: CPU%d up fail: %d\n",
					__func__, cpu, ret);
				continue;
			} else {
				cpumask_clear_cpu(cpu, &out_cpus);
			}
		}
	}
	mutex_unlock(&hotplug_mutex);

	return;
}

static unsigned int * bak_trans_costs;
static unsigned int * bak_trans_delays;
static unsigned int bak_ncosts, bak_ndelays;
static unsigned int bak_sampling_rate;

static int fb_state_change(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct fb_event *evdata = data;
	unsigned int blank;
	unsigned long flags;

	if (val != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_POWERDOWN:
		bak_sampling_rate = dbs_tuners_ins.sampling_rate;
		dbs_tuners_ins.sampling_rate = LOWPOWER_SAMPLING_RATE;
		spin_lock_irqsave(&trans_lock, flags);
		bak_trans_costs = trans_costs;
		bak_trans_delays = trans_delays;
		bak_ncosts = ntrans_costs;
		bak_ndelays = ntrans_delays;

		trans_costs = lowpower_trans_costs;
		trans_delays = lowpower_trans_delays;
		ntrans_costs = ARRAY_SIZE(lowpower_trans_costs);
		ntrans_delays = ARRAY_SIZE(lowpower_trans_delays);
		spin_unlock_irqrestore(&trans_lock, flags);
		lcd_is_on = false;
		break;
	case FB_BLANK_UNBLANK:
		/*
		 * LCD blank CPU qos is set by exynos-ikcs-cpufreq
		 * This line of code release max limit when LCD is
		 * turned on.
		 */
#ifdef CONFIG_ARM_EXYNOS_IKS_CLUSTER
		if (pm_qos_request_active(&max_cpu_qos_blank))
			pm_qos_remove_request(&max_cpu_qos_blank);
#endif
		dbs_tuners_ins.sampling_rate = bak_sampling_rate;
		spin_lock_irqsave(&trans_lock, flags);
		trans_costs = bak_trans_costs;
		trans_delays = bak_trans_delays;
		ntrans_costs = bak_ncosts;
		ntrans_delays = bak_ndelays;
		spin_unlock_irqrestore(&trans_lock, flags);

		lcd_is_on = true;
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block fb_block = {
	.notifier_call = fb_state_change,
};
#endif	// CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

/* cpufreq_programmable Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(up_threshold, up_threshold);
show_one(down_differential, down_differential);
show_one(ignore_nice_load, ignore_nice);
show_one(io_is_busy, io_is_busy);
show_one(cur_up_step, up_step_level);
show_one(cur_down_step, down_step_level);
show_one(cur_ratemult_up, up_ratemult);
show_one(cur_ratemult_down, down_ratemult);

static ssize_t show_trans_step_table(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	int i;
	ssize_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&trans_lock, flags);

	ret += sprintf(buf, "%d(UP), %d(DOWN), %d(BOTH)\n", SCALE_UP, SCALE_DOWN, SCALE_BOTH);

	for (i = 0; i < ntrans_steps ; i+=2)
		ret += sprintf(buf + ret, "%u:%u ", trans_steps[i],
			       trans_steps[i+1]);

	ret += sprintf(buf + ret, "\n");
	spin_unlock_irqrestore(&trans_lock, flags);
	return ret;
}

static ssize_t store_trans_step_table(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	int ret;
	const char *cp;
	unsigned int *new_trans_table = NULL;
	int ntokens = 1;
	int i;
	unsigned long flags;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if ((ntokens & 0x1))	// if odd, invalid.
		goto err_inval;

	new_trans_table = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!new_trans_table) {
		ret = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &new_trans_table[i]) != 1)
			goto err_inval;

		if(i&0x1) {	// step direction
			if(new_trans_table[i] < SCALE_UP || new_trans_table[i] > SCALE_BOTH)
				goto err_inval;
		}

		if(!(i&0x1) && i >= 2) {
			if(new_trans_table[i-2] >= new_trans_table[i])
				goto err_inval;
		}

		i++;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_inval;

	spin_lock_irqsave(&trans_lock, flags);
	if (trans_steps != default_trans_steps)
		kfree(trans_steps);
	trans_steps = new_trans_table;
	ntrans_steps = ntokens;
	spin_unlock_irqrestore(&trans_lock, flags);
	return count;

err_inval:
	ret = -EINVAL;
err:
	if(new_trans_table) kfree(new_trans_table);
	return ret;

}

static ssize_t show_trans_cost_table(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	int i;
	ssize_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&trans_lock, flags);

	ret += sprintf(buf, "{Freq} Up DownDiff:\n");

	for (i = 0; i < ntrans_costs ; i++)
		ret += sprintf(buf + ret, "%u%s", trans_costs[i],
			       (i % 3 == 0x2) ? (":"):(" "));

	ret += sprintf(buf + ret, "\n");
	spin_unlock_irqrestore(&trans_lock, flags);
	return ret;
}

static ssize_t store_trans_cost_table(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	int ret;
	const char *cp;
	unsigned int *new_trans_table = NULL;
	int ntokens = 1;
	int i;
	unsigned long flags;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if ((ntokens%3) != 2)	// invalid.
		goto err_inval;

	new_trans_table = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!new_trans_table) {
		ret = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &new_trans_table[i]) != 1)
			goto err_inval;

		if((i%3)==1) {	// up threshold/down threshold
			if(new_trans_table[i-1] <= new_trans_table[i])
				goto err_inval;
			if(new_trans_table[i-1] > MAX_FREQUENCY_UP_THRESHOLD ||
				new_trans_table[i-1] < MIN_FREQUENCY_UP_THRESHOLD)
				goto err_inval;
			if(new_trans_table[i] > 100 || new_trans_table[i] < 0)
				goto err_inval;
		}

		if((i%3)==2 && i > 3) {
			if(new_trans_table[i-3] >= new_trans_table[i])
				goto err_inval;
		}

		i++;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_inval;

	spin_lock_irqsave(&trans_lock, flags);
	if (trans_costs != default_trans_costs)
		kfree(trans_costs);
	trans_costs = new_trans_table;
	ntrans_costs = ntokens;
	spin_unlock_irqrestore(&trans_lock, flags);
	return count;

err_inval:
	ret = -EINVAL;
err:
	if(new_trans_table) kfree(new_trans_table);
	return ret;

}

static ssize_t show_trans_delay_table(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	int i;
	ssize_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&trans_lock, flags);

	ret += sprintf(buf, "{Freq} Uprate Downrate:\n");

	for (i = 0; i < ntrans_delays ; i++)
		ret += sprintf(buf + ret, "%u%s", trans_delays[i],
			       (i % 3 == 0x2) ? (":"):(" "));

	ret += sprintf(buf + ret, "\n");
	spin_unlock_irqrestore(&trans_lock, flags);
	return ret;
}

static ssize_t store_trans_delay_table(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	int ret;
	const char *cp;
	unsigned int *new_trans_table = NULL;
	int ntokens = 1;
	int i;
	unsigned long flags;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if ((ntokens%3) != 2)	// invalid.
		goto err_inval;

	new_trans_table = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!new_trans_table) {
		ret = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &new_trans_table[i]) != 1)
			goto err_inval;

		if((i%3)==1) {
			if(new_trans_table[i-1] > 100 || new_trans_table[i-1] < 0)
				goto err_inval;
			if(new_trans_table[i] > 100 || new_trans_table[i] < 0)
				goto err_inval;
		}

		if((i%3)==2 && i > 3) {
			if(new_trans_table[i-3] >= new_trans_table[i])
				goto err_inval;
		}

		i++;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_inval;

	spin_lock_irqsave(&trans_lock, flags);
	if (trans_delays != default_trans_delays)
		kfree(trans_delays);
	trans_delays = new_trans_table;
	ntrans_delays = ntokens;
	spin_unlock_irqrestore(&trans_lock, flags);
	return count;

err_inval:
	ret = -EINVAL;
err:
	if(new_trans_table) kfree(new_trans_table);
	return ret;

}

/**
 * update_sampling_rate - update sampling rate effective immediately if needed.
 * @new_rate: new sampling rate
 *
 * If new rate is smaller than the old, simply updaing
 * dbs_tuners_int.sampling_rate might not be appropriate. For example,
 * if the original sampling_rate was 1 second and the requested new sampling
 * rate is 10 ms because the user needs immediate reaction from april
 * governor, but not sure if higher frequency will be required or not,
 * then, the governor may change the sampling rate too late; up to 1 second
 * later. Thus, if we are reducing the sampling rate, we need to make the
 * new value effective immediately.
 */
static void update_sampling_rate(unsigned int new_rate)
{
	int cpu;

	dbs_tuners_ins.sampling_rate = new_rate
				     = max(new_rate, min_sampling_rate);

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct cpu_dbs_info_s *dbs_info;
		unsigned long next_sampling, appointed_at;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;
		dbs_info = &per_cpu(od_cpu_dbs_info, policy->cpu);
		cpufreq_cpu_put(policy);

		mutex_lock(&dbs_info->timer_mutex);

		if (!delayed_work_pending(&dbs_info->work)) {
			mutex_unlock(&dbs_info->timer_mutex);
			continue;
		}

		next_sampling  = jiffies + usecs_to_jiffies(new_rate);
		appointed_at = dbs_info->work.timer.expires;


		if (time_before(next_sampling, appointed_at)) {

			mutex_unlock(&dbs_info->timer_mutex);
			cancel_delayed_work_sync(&dbs_info->work);
			mutex_lock(&dbs_info->timer_mutex);

			schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work,
						 usecs_to_jiffies(new_rate));

		}
		mutex_unlock(&dbs_info->timer_mutex);
	}
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	update_sampling_rate(input);
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];

	}
	return count;
}


define_one_global_ro(sampling_rate_min);
define_one_global_ro(up_threshold);
define_one_global_ro(down_differential);
define_one_global_ro(cur_up_step);
define_one_global_ro(cur_down_step);
define_one_global_ro(cur_ratemult_up);
define_one_global_ro(cur_ratemult_down);

define_one_global_rw(sampling_rate);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(io_is_busy);
define_one_global_rw(trans_step_table);
define_one_global_rw(trans_cost_table);
define_one_global_rw(trans_delay_table);

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
static int cpu_util[4];

static ssize_t show_cpu_utilization(struct kobject *kobj,
					struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d %d %d\n", cpu_util[0], cpu_util[1],
				cpu_util[2], cpu_util[3]);
}

define_one_global_ro(cpu_utilization);
#endif // CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&down_differential.attr,
	&ignore_nice_load.attr,
	&io_is_busy.attr,
	&cur_up_step.attr,
	&cur_down_step.attr,
	&cur_ratemult_up.attr,
	&cur_ratemult_down.attr,
	&trans_step_table.attr,
	&trans_cost_table.attr,
	&trans_delay_table.attr,
#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	&cpu_utilization.attr,
#endif // CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "april",
};

/************************** sysfs end ************************/

static void dbs_freq_increase(struct cpufreq_policy *p, unsigned int freq)
{
#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	bool hotplug_in = false;
#endif
	if (p->cur == p->max)
		return;

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	/*
	 * If boost level is sustaning over than BOOST_LV_CNT, replace frequency
	 * with HOTPLUG_TRANS_H level and try cpu hotplug in after changing frequency
	 */
	if ((consecutive_boost_level > BOOST_LV_CNT) &&
		(freq > HOTPLUG_TRANS_H)) {
		freq = HOTPLUG_TRANS_H;
		hotplug_in = true;
	}
#endif

	__cpufreq_driver_target(p, freq, CPUFREQ_RELATION_H);

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	/*
	 * Hotplug in case :
	 * - Boost level is continuing over than BOOST_LV_CNT
	 * - Prior to cpu hotplug, frequency should be changed below 1.6Ghz
	 */
	if (!cpumask_empty(&out_cpus) && hotplug_in) {
				hotplug_out = false;
				schedule_work_on(0, &hotplug_work);
	}
#endif

}

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	unsigned int max_load_freq, up_load_freq, down_load_freq;
	unsigned int possible_scaling = SCALE_NONE;

	struct cpufreq_policy *policy;
	unsigned int j;
	u64 now;
#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	unsigned int cpu_util_sum = 0;
#endif

	policy = this_dbs_info->cur_policy;

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate, we look for a the lowest
	 * frequency which can sustain the load while keeping idle time over
	 * 30%. If such a frequency exist, we try to decrease to this frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of current frequency
	 */

	/* Get Absolute Load - in terms of freq */
	max_load_freq = 0;

	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
		unsigned int idle_time, wall_time, iowait_time;
		unsigned int load, load_freq;
		int freq_avg;

		j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
		cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

		wall_time = (unsigned int)
			(cur_wall_time - j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int)
			(cur_idle_time - j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		iowait_time = (unsigned int)
			(cur_iowait_time - j_dbs_info->prev_cpu_iowait);
		j_dbs_info->prev_cpu_iowait = cur_iowait_time;

		if (dbs_tuners_ins.ignore_nice) {
			u64 cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
					 j_dbs_info->prev_cpu_nice;
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		/*
		 * For the purpose of april, waiting for disk IO is an
		 * indication that you're performance critical, and not that
		 * the system is actually idle. So subtract the iowait time
		 * from the cpu idle time.
		 */

		if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
			idle_time -= iowait_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;
#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
		cpu_util[j] = load;
		cpu_util_sum += load;
#endif
		freq_avg = __cpufreq_driver_getavg(policy, j);
		if (freq_avg <= 0)
			freq_avg = policy->cur;

		load_freq = load * freq_avg;
		if (load_freq > max_load_freq)
			max_load_freq = load_freq;
	}

	this_dbs_info->longterm_load_freq += max_load_freq;
	this_dbs_info->longterm_count++;

	now = get_jiffies_64();

	if(now - dbs_tuners_ins.validate_timestamp
		< dbs_tuners_ins.longterm_jiffies) { // Not yet for longterm duration
		possible_scaling = dbs_tuners_ins.shortterm_direction;

		up_load_freq = (possible_scaling & SCALE_UP) ? (max_load_freq) : (0);
		down_load_freq = (possible_scaling & SCALE_DOWN) ? (max_load_freq) :
			(100 * policy->cur);
	} else {
		possible_scaling = SCALE_BOTH;

		up_load_freq = (dbs_tuners_ins.shortterm_direction & SCALE_UP) ?
			(max_load_freq) :
			(this_dbs_info->longterm_load_freq / this_dbs_info->longterm_count);

		down_load_freq = (dbs_tuners_ins.shortterm_direction & SCALE_DOWN) ?
			(max_load_freq) :
			(this_dbs_info->longterm_load_freq / this_dbs_info->longterm_count);

		this_dbs_info->longterm_load_freq = 0;
		this_dbs_info->longterm_count = 0;
	}

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
#ifdef ENABLE_HOTPLUG_OUT_H
		/* Hotplug out case : Frequency stay over maximum quad level */
		if ((policy->cur < HOTPLUG_TRANS_H) ||
				num_online_cpus() <= HOTPLUG_TRANS_H_CPUS)
			goto skip_hotplug_out_1;

		/*
		 * If next transition is descending from over HOTPLUG_TRANS_H
		 * do not try hotplug out
		 */
		if (up_load_freq <= dbs_tuners_ins.up_threshold * policy->cur)
			goto skip_hotplug_out_1;

		/*
		 * If policy->cur >= 1.6Ghz(HOTPLUG_TRANS_H) and next transition is
		 * ascending check cpu_util value for each online cpu.
		 * If cpu_util is less than 10%(HOTPLUG_OUT_LOAD) for 3(HOTPLUG_OUT_CNT_H)
		 * times sampling rate(100ms), plugged out on this cpu.
		 */
		for_each_online_cpu(j) {
			/* core 0 must not be hotplugged out */
			if (j == 0)
				continue;
			if (cpumask_weight(cpu_online_mask) - cpumask_weight(&to_be_out_cpus) <=
					HOTPLUG_TRANS_H_CPUS)
				break;

			if (cpu_util[j] < HOTPLUG_OUT_LOAD) {
				per_cpu(hotplug_out_cnt_h, j)++;

				if (per_cpu(hotplug_out_cnt_h, j) > HOTPLUG_OUT_CNT_H) {
					cpumask_set_cpu(j, &to_be_out_cpus);
					per_cpu(hotplug_out_cnt_h, j) = 0;
				}
			} else {
				/* Reset out trigger counter */
				per_cpu(hotplug_out_cnt_h, j) = 0;
			}
		}
		/*
		 * Hotplug Out:
		 * - Frequency is 1.6/1.7Ghz
		 * - Some cpu's utilization is less than 10%
		 */
	if (!cpumask_empty(&to_be_out_cpus)) {
				hotplug_out = false;
				schedule_work_on(0, &hotplug_work);
	}

	skip_hotplug_out_1:
		/*
		 * Increase consecutive_boost_level if policy->cur is higher than
		 * 1.6Ghz(HOTPLUG_TRANS_H) for consecutive period.
		 * If not, reset this variable.
		 */
		if (policy->cur > HOTPLUG_TRANS_H)
			consecutive_boost_level++;
		else
			consecutive_boost_level = 0;

#endif	// ENABLE_HOTPLUG_OUT_H
#endif	// CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG

	if(up_load_freq > dbs_tuners_ins.up_threshold * policy->cur) {
#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
		/*
		 * Hotplug In:
		 */
		if (!cpumask_empty(&out_cpus) && policy->cur < HOTPLUG_TRANS_H) {
				hotplug_out = false;
				schedule_work_on(0, &hotplug_work);
		}
#endif
		dbs_freq_increase(policy, dbs_tuners_ins.up_step_level);
		return;
	}


#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	/*
	 * Hotplug Out:
	 * - Frequency stay at lowest level
	 */
	if ((policy->cur > HOTPLUG_TRANS_L) ||
			num_online_cpus() <= HOTPLUG_TRANS_L_CPUS ||
			lcd_is_on)
		goto skip_hotplug_out_2;

	if (cpu_util_sum <
		dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) {

		hotplug_out_cnt_l++;

		if (hotplug_out_cnt_l > HOTPLUG_OUT_CNT_L) {
			cpumask_setall(&to_be_out_cpus);
			cpumask_clear_cpu(0, &to_be_out_cpus);
			hotplug_out_cnt_l = 0;
		}
	} else {
		/* Reset out trigger counter */
		hotplug_out_cnt_l = 0;
	}

	if (!cpumask_empty(&to_be_out_cpus)) {
		hotplug_out = true;
		schedule_work_on(0, &hotplug_work);
	}

skip_hotplug_out_2:
#endif	// CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG

	/* Check for frequency decrease */
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min)
		return;

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (down_load_freq <
	    (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
	     policy->cur) {
		unsigned int freq_next;
		freq_next = down_load_freq /
				(dbs_tuners_ins.up_threshold -
				 dbs_tuners_ins.down_differential);

		if (freq_next < dbs_tuners_ins.down_step_level)
			freq_next = dbs_tuners_ins.down_step_level;

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
		/*
		 * Hotplug In:
		 * - Descending frequency from down_step_level try hotplug in
		 * first to reduce pluging out latency and then down frequency.
		 */
		if (policy->cur <= HOTPLUG_TRANS_H_BTM && !cpumask_empty(&out_cpus)) {
				hotplug_out = false;
				schedule_work_on(0, &hotplug_work);
		}
#endif // CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG


		__cpufreq_driver_target(policy, freq_next,
				CPUFREQ_RELATION_L);

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
		/*
		 * Hotplug In case : Decrease frequency to over down step level
		 * If 1.8Ghz -> 1.7Ghz transition, need to keep current hotplug
		 * state. Do not perform below routine.
		 * If next level is descending below 1.6Ghz(HOTPLUG_TRANS_H), try
		 * hotplug in on all plugged out cpus.
		 */
		if (policy->cur > HOTPLUG_TRANS_H_BTM && freq_next <= HOTPLUG_TRANS_H
			&& !cpumask_empty(&out_cpus)) {
				hotplug_out = false;
				schedule_work_on(0, &hotplug_work);
		}
#endif // CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	}


}

#define KHz2MHz(val)	(val/1000)
#define MHz2KHz(val)	(val*1000)
#define ENCODE_PARAM(lval,rval) ((((0xffff)&(lval))<<16)|((0xffff)&(rval)))
#define DECODE_PARAM(target, lval, rval) do {	\
			lval = (((target) & (0xffff0000))>>16);	\
			rval = ((target) & (0xffff));				\
		} while(0)

static unsigned int freq_to_transstep(unsigned int freq) {
	int i;
	unsigned int lval = 0xffff;
	unsigned int rval = 0x0;
	unsigned long flags;

	spin_lock_irqsave(&trans_lock, flags);

	/* Find min up step */
	for (i = 0;	i < ntrans_steps; i+=2) {
		if(trans_steps[i] > freq && trans_steps[i+1] != SCALE_DOWN) {
			lval = KHz2MHz(trans_steps[i]);
			break;
		}
	}

	/* Find max down step */
	for (i = ntrans_steps-2; i >= 0; i-=2) {
		if(trans_steps[i] < freq && trans_steps[i+1] != SCALE_UP) {
			rval = KHz2MHz(trans_steps[i]);
			break;
		}
	}

	spin_unlock_irqrestore(&trans_lock, flags);

	return ENCODE_PARAM(lval, rval);
}


#define freq_to_trans(attr)	\
static unsigned int freq_to_trans##attr (unsigned int freq) {	\
	int i;														\
	unsigned int lval = 0xffff;									\
	unsigned int rval = 0x0;									\
	unsigned long flags;											\
\
	spin_lock_irqsave(&trans_lock, flags);						\
\
	for(i = 0; i < ntrans_##attr - 2 && freq >= trans_##attr[i+2]; i+=3)	\
		;														\
\
	lval = trans_##attr[i];										\
	rval = trans_##attr[i+1];									\
\
	spin_unlock_irqrestore(&trans_lock, flags);					\
\
	return ENCODE_PARAM(lval, rval);							\
}

freq_to_trans(costs);
freq_to_trans(delays);
/* Can be extended by additional feature */



static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	unsigned int prev = dbs_info->cur_policy->cur;
	unsigned int cur;
	int delay;

	mutex_lock(&dbs_info->timer_mutex);
	dbs_check_cpu(dbs_info);

	cur = dbs_info->cur_policy->cur;

	if(prev != cur) { // If scaled, frequency attr should be reset
		unsigned int tmpVal, lval, rval;

		/* 1. Get trans step attributes */
		tmpVal = freq_to_transstep(cur);
		DECODE_PARAM(tmpVal, lval, rval);
		dbs_tuners_ins.up_step_level = MHz2KHz(lval);
		dbs_tuners_ins.down_step_level = MHz2KHz(rval);

		if(dbs_tuners_ins.up_step_level > dbs_info->cur_policy->max)
			dbs_tuners_ins.up_step_level = dbs_info->cur_policy->max;

		if(dbs_tuners_ins.down_step_level < dbs_info->cur_policy->min)
			dbs_tuners_ins.down_step_level = dbs_info->cur_policy->min;

		/* 2. Get trans cost attributes */
		tmpVal = freq_to_transcosts(cur);
		DECODE_PARAM(tmpVal, lval, rval);
		dbs_tuners_ins.up_threshold = lval;
		dbs_tuners_ins.down_differential = rval;

		if(dbs_tuners_ins.up_threshold < MIN_FREQUENCY_UP_THRESHOLD)
			dbs_tuners_ins.up_threshold = MIN_FREQUENCY_UP_THRESHOLD;
		if(dbs_tuners_ins.up_threshold > MAX_FREQUENCY_UP_THRESHOLD)
			dbs_tuners_ins.up_threshold = MAX_FREQUENCY_UP_THRESHOLD;
		if(dbs_tuners_ins.up_threshold < dbs_tuners_ins.down_differential)
			dbs_tuners_ins.down_differential = 0;

		/* 3. Get trans delay attributes */
		tmpVal = freq_to_transdelays(cur);
		DECODE_PARAM(tmpVal, lval, rval);
		dbs_tuners_ins.up_ratemult = lval;
		dbs_tuners_ins.down_ratemult = rval;

		if(dbs_tuners_ins.up_ratemult < MIN_SAMPLE_RATE_MULT)
			dbs_tuners_ins.up_ratemult = MIN_SAMPLE_RATE_MULT;
		if(dbs_tuners_ins.up_ratemult > MAX_SAMPLE_RATE_MULT)
			dbs_tuners_ins.up_ratemult = MAX_SAMPLE_RATE_MULT;
		if(dbs_tuners_ins.down_ratemult < MIN_SAMPLE_RATE_MULT)
			dbs_tuners_ins.down_ratemult = MIN_SAMPLE_RATE_MULT;
		if(dbs_tuners_ins.down_ratemult > MAX_SAMPLE_RATE_MULT)
			dbs_tuners_ins.down_ratemult = MAX_SAMPLE_RATE_MULT;


		if (dbs_tuners_ins.up_ratemult > dbs_tuners_ins.down_ratemult) {
			dbs_tuners_ins.shortterm_direction  = SCALE_DOWN;

			dbs_tuners_ins.longterm_jiffies		= usecs_to_jiffies(
				dbs_tuners_ins.up_ratemult * dbs_tuners_ins.sampling_rate);
			dbs_tuners_ins.shortterm_jiffies	= usecs_to_jiffies(
				dbs_tuners_ins.down_ratemult * dbs_tuners_ins.sampling_rate);
		} else {
			if (dbs_tuners_ins.up_ratemult < dbs_tuners_ins.down_ratemult)
				dbs_tuners_ins.shortterm_direction  = SCALE_UP;
			else
				dbs_tuners_ins.shortterm_direction  = SCALE_BOTH;

			dbs_tuners_ins.longterm_jiffies		= usecs_to_jiffies(
				dbs_tuners_ins.down_ratemult * dbs_tuners_ins.sampling_rate);
			dbs_tuners_ins.shortterm_jiffies	= usecs_to_jiffies(
				dbs_tuners_ins.up_ratemult * dbs_tuners_ins.sampling_rate);
		}

		dbs_tuners_ins.validate_timestamp = get_jiffies_64();

		prev = dbs_info->cur_policy->cur;

	}

#if 0
	printk("%dKHz %d~%d %d/%d (%d/%d)[%ld.%ld]\n", cur/1000,
		dbs_tuners_ins.up_step_level / 1000,
		dbs_tuners_ins.down_step_level / 1000,
		dbs_tuners_ins.up_threshold, dbs_tuners_ins.down_differential,
		(int)(dbs_tuners_ins.shortterm_jiffies), (int)(dbs_tuners_ins.longterm_jiffies),
		(int)(dbs_tuners_ins.up_ratemult), (int)(dbs_tuners_ins.down_ratemult));
#endif

	delay = dbs_tuners_ins.shortterm_jiffies;

	if (num_online_cpus() > 1)
	delay -= jiffies % delay;

	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = dbs_tuners_ins.shortterm_jiffies;

	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	dbs_info->sample_type = DBS_SHORT_SAMPLE;
	INIT_DELAYED_WORK(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
}

/*
 * Not all CPUs want IO time to be accounted as busy; this dependson how
 * efficient idling at a higher frequency/voltage is.
 * Pavel Machek says this is not so for various generations of AMD and old
 * Intel systems.
 * Mike Chan (androidlcom) calis this is also not true for ARM.
 * Because of this, whitelist specific known (series) of CPUs by default, and
 * leave all others up to the user.
 */
static int should_io_be_busy(void)
{
#if defined(CONFIG_X86)
	/*
	 * For Intel, Core 2 (model 15) andl later have an efficient idle.
	 */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
	    boot_cpu_data.x86 == 6 &&
	    boot_cpu_data.x86_model >= 15)
		return 1;
#endif
	return 0;
}

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice)
				j_dbs_info->prev_cpu_nice =
						kcpustat_cpu(j).cpustat[CPUTIME_NICE];

			if (dbs_enable == 1)
				dbs_tuners_ins.validate_timestamp = usecs_to_jiffies(j_dbs_info->prev_cpu_wall);
		}
		this_dbs_info->cpu = cpu;

		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);		// 10,000
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);

			dbs_tuners_ins.longterm_jiffies = usecs_to_jiffies(
				dbs_tuners_ins.up_ratemult * dbs_tuners_ins.sampling_rate);

			dbs_tuners_ins.shortterm_jiffies = usecs_to_jiffies(
				dbs_tuners_ins.up_ratemult * dbs_tuners_ins.sampling_rate);

			printk("%s: %dms, %d/%d", __func__, dbs_tuners_ins.sampling_rate,
				dbs_tuners_ins.longterm_jiffies, dbs_tuners_ins.shortterm_jiffies);

			dbs_tuners_ins.io_is_busy = should_io_be_busy();
		}
		mutex_unlock(&dbs_mutex);
#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
		cpumask_clear(&out_cpus);
		cpumask_clear(&to_be_out_cpus);

		mutex_init(&hotplug_mutex);
		INIT_WORK(&hotplug_work, do_hotplug);
		consecutive_boost_level = 0;
#endif

		mutex_init(&this_dbs_info->timer_mutex);
		dbs_timer_init(this_dbs_info);
		break;

	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
		mutex_destroy(&this_dbs_info->timer_mutex);
		dbs_enable--;
		mutex_unlock(&dbs_mutex);
		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
		cancel_work_sync(&hotplug_work);
		mutex_destroy(&hotplug_mutex);
		for_each_cpu(j, &out_cpus)
			cpu_up(j);

		cpumask_clear(&out_cpus);
		cpumask_clear(&to_be_out_cpus);
		consecutive_boost_level = 0;
#endif

		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

static int __init cpufreq_gov_dbs_init(void)
{
	u64 idle_time;
	int cpu = get_cpu();

	idle_time = get_cpu_idle_time_us(cpu, NULL);
	put_cpu();
	if (idle_time != -1ULL) {
		dbs_tuners_ins.sampling_rate = LATENCY_MULTIPLIER * MIN_LATENCY_MULTIPLIER;
		dbs_tuners_ins.shortterm_direction = SCALE_BOTH;
		/*
		 * In nohz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		min_sampling_rate = MIN_SAMPLE_RATE;
	} else {
		/* For correct statistics, we need 10 ticks for each measure */
		min_sampling_rate =
			MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
	}

	spin_lock_init(&trans_lock);

#ifdef CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG
	fb_register_client(&fb_block);

	lcd_is_on = true;
#endif

	return cpufreq_register_governor(&cpufreq_gov_april);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_april);
}


MODULE_AUTHOR("Johnlay <jonglae.park@samsung.com>");
MODULE_AUTHOR("Johnlay <jonglae.park@samsung.com>");
MODULE_DESCRIPTION("'cpufreq_april' - A dynamic cpufreq governor for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_APRIL
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
