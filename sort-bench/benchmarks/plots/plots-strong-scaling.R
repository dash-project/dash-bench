library(dplyr)
library(ggplot2)
library(RColorBrewer)

## Summarizes data.
## Gives count, mean, standard deviation, standard error of the mean, and
## confidence interval (default 95%).
##   data: a data frame.
##   measurevar: the name of a column that contains the variable to be summariezed
##   groupvars: a vector containing names of columns that contain grouping variables
##   na.rm: a boolean that indicates whether to ignore NA's
##   conf.interval: the percent range of the confidence interval (default is 95%)
summarySE <- function(data=NULL, measurevar, groupvars=NULL, na.rm=FALSE,
                      conf.interval=.95, .drop=TRUE) {
    # New version of length which can handle NA's: if na.rm==T, don't count them
    length2 <- function (x, na.rm=FALSE) {
        if (na.rm) sum(!is.na(x))
        else       length(x)
    }

    # This does the summary. For each group's data frame, return a vector with
    # N, mean, and sd
    datac <- plyr::ddply(data, groupvars, .drop=.drop,
                         .fun = function(xx, col) {
                             c(N    = length2(xx[[col]], na.rm=na.rm),
                               mean = mean   (xx[[col]], na.rm=na.rm),
                               sd   = sd     (xx[[col]], na.rm=na.rm),
                               min  = min    (xx[[col]], na.rm=na.rm),
                               max  = max    (xx[[col]], na.rm=na.rm)
                               )
                         },
                         measurevar
                         )

    # Rename the "mean" column
    datac <- plyr::rename(datac, c("mean" = measurevar))

    datac$se <- datac$sd / sqrt(datac$N)  # Calculate standard error of the mean

    # Confidence interval multiplier for standard error
    # Calculate t-statistic for confidence interval:
    # e.g., if conf.interval is .95, use .975 (above/below), and use df=N-1
    ciMult <- qt(conf.interval/2 + .5, datac$N-1)
    datac$ci <- datac$se * ciMult

    return(datac)
}

plotLineChart <- function(data, title="") {

# The errorbars overlapped, so use position_dodge to move them horizontally
    pd <- position_dodge(0.1) # move them .05 to the left and right

    ggplot(data, aes(x=NTasks, y=Time, colour=Test.Case, group=Test.Case)) +
        geom_errorbar(aes(ymin=Time-ci, ymax=Time+ci), colour="black", width=.1, position=pd) +
        geom_line(position=pd) +
        geom_point(position=pd, size=3, shape=21, fill="white") + # 21 is filled circle
        xlab("NTasks") +
            ylab("Time") +
            scale_colour_hue(name="Implementation",    # Legend label, use darker colors
                             breaks=c("dash.x", "mpi.x"),
                             labels=c("DASH", "MPI"),
                             l=40) +                    # Use darker colors, lightness=40
    ggtitle(title) +
        expand_limits(y=0) +                        # Expand y range
        scale_y_continuous(breaks=0:20*4) +         # Set tick every 4
            theme_bw() +
                theme(legend.justification=c(1,0),
                      legend.position=c(1,0))               # Position legend in bottom right

}

plotBarChart <- function(data, title="") {

    data$NTasks <- factor(data$NTasks)

    ggplot(data, aes(x=NTasks, y=Time, fill=Test.Case)) +
    geom_bar(position=position_dodge(), stat="identity",
             colour="black", # Use black outlines,
             size=.3) +      # Thinner lines
    geom_errorbar(aes(ymin=Time-ci, ymax=Time+ci),
                  size=.3,    # Thinner lines
                  width=.2,
                  position=position_dodge(.9)) +
    xlab("NTasks") +
    ylab("Time") +
    scale_fill_brewer(name="Implementation", # Legend label, use darker colors
                   breaks=c("dash.x", "mpi.x"),
                   labels=c("DASH", "MPI"),
                   palette="GnBu") +
    ggtitle(title) +
    scale_y_continuous(breaks=0:20*4) +
    theme_bw()
}

strongScaling.data <- read.csv("../summary/dist-memory/strong-scaling.csv", header=TRUE, strip.white=TRUE)

strongScaling.view <- summarySE(strongScaling.data,
                    measurevar="Time",
                    groupvars=c("NTasks", "Test.Case"),
                    na.rm=TRUE)

title <- paste("Strong Scaling (Memory Size:", strongScaling.data[1, "Size"], "MB)");

#sizeScaling.cores.view <- strongScaling.view %>%
#    filter((Size < 25000) &
#            (Test.Case == "dash.x" |
#            Test.Case == "openmp.x" |
#            Test.Case == "tbb-lowlevel.x" |
#            Test.Case == "tbb-highlevel.x"))

pdf(file="strong-scaling.pdf")

plotLineChart(strongScaling.view, title)
plotBarChart(strongScaling.view, title)

dev.off()


