library(dplyr)

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

args = commandArgs(trailingOnly=TRUE)

if (length(args)==0) {
    stop("Usage: ./plots.R <infile> <outfile>", call.=FALSE)
} else if (length(args)==1) {
    filename <- basename(args[1])
    filename <- sub("^([^.]*).*", "\\1", filename)
    filename <- paste(filename, ".pdf", sep="")
    args[2] <- filename
}

csvFile <- args[1]
outfile <- args[2]

my.data <- read.csv(csvFile, header=TRUE, strip.white=TRUE)

my.agg <- summarySE(my.data,
                    measurevar="Time",
                    groupvars=c("Size", "Test.Case"),
                    na.rm=TRUE)

my.shm <- my.agg %>%
    filter(Size < 25500 &
           (Test.Case == "dash.x" |
            Test.Case == "openmp.x" |
            Test.Case == "tbb-lowlevel.x" |
            Test.Case == "tbb-highlevel.x"))

library(ggplot2)

pdf(file=outfile)

# The errorbars overlapped, so use position_dodge to move them horizontally
pd <- position_dodge(0.1) # move them .05 to the left and right

ggplot(my.shm, aes(x=Size, y=Time, colour=Test.Case, group=Test.Case)) +
    geom_errorbar(aes(ymin=Time-ci, ymax=Time+ci), colour="black", width=.1, position=pd) +
    geom_line(position=pd) +
    geom_point(position=pd, size=3, shape=21, fill="white") + # 21 is filled circle
    xlab("Size (MB)") +
        ylab("Time") +
        scale_colour_hue(name="Implementation",    # Legend label, use darker colors
                         breaks=c("dash.x", "tbb-lowlevel.x", "tbb-highlevel.x", "openmp.x"),
                         labels=c("DASH", "TBB (Low)", "TBB (High)", "OpenMP"),
                         l=40) +                    # Use darker colors, lightness=40
ggtitle("Sort Benchmark") +
    expand_limits(y=0) +                        # Expand y range
    scale_y_continuous(breaks=0:20*4) +         # Set tick every 4
        theme_bw() +
            theme(legend.justification=c(1,0),
                  legend.position=c(1,0))               # Position legend in bottom right

dev.off()


