#!/usr/bin/env Rscript

library(dplyr)
library(tableHTML)

args = commandArgs(trailingOnly=TRUE)

if (length(args) < 2) {
    stop("Usage: <script> <infile> <withHeader>", call.=FALSE)
}

fileName <- basename(args[1])
withHeader <- basename(args[2])
withHeader <- withHeader == "y"

fileName <- sub("^(.*)(\\.[a-z0-9].*$)", "\\1", fileName)
dirname <- dirname(args[1])

outSvg <- paste(fileName, "-plot.csv", sep="")
outSvg <- paste(dirname, outSvg, sep="/")

# read csv
data.raw <- read.csv(args[1], header=withHeader, strip.white=TRUE)
# name all columns regardless of the already existing header
colnames(data.raw) <- c("context", "unit", "start", "end", "state")
# reorder columns
cols <- c("state","unit","start","end")
data.raw <- data.raw[cols]

#discard detail traces about partition border finding
data.svg <- data.raw %>% filter(!grepl("[0-9]\\.[0-9].*", state))
#we extract the phase and finally order by unit and phase
data.svg <- mutate(data.svg, phase=gsub("([0-9]+)(:.*)$", "\\1", data.svg$state))
data.svg <- arrange(data.svg, as.numeric(unit), as.numeric(phase));

# only write certain columns to csv and omit the header
write.table(data.svg[,cols], outSvg, row.names = F, col.names = F, sep = ",")

# print out the generated svg filename
cat(outSvg, "\n")


# Calculate the summary here (mean, median, etc.) for all phase among all processors
outSummary <- paste(fileName, "-summary.html", sep="")
outSummary <- paste(dirname, outSummary, sep="/")

data.summary <- data.svg[cols]
data.summary$time <- (data.summary$end - data.summary$start) *1E-6
data.summary <- data.summary[c("state", "time")]

if(!exists("summarySE", mode="function")) {
    # Let us source the summary.R script
    initial.options <- commandArgs(trailingOnly = FALSE)
    file.arg.name <- "--file="
    script.name <- sub(file.arg.name, "", initial.options[grep(file.arg.name, initial.options)])
    script.basename <- dirname(script.name)
    other.name <- file.path(script.basename, "../summary.R")
    source(other.name)
}

data.summary <- summarySE(data.summary,
                          measurevar="time",
                          groupvars=c("state"),
                          na.rm=TRUE)

# order in reverse order, so slow operations come first
data.summary <- arrange(data.summary, -as.numeric(time));

# only write certain columns to csv and omit the header
#write.table(data.summary[,c("state", "N", "time", "sd")], outSummary, row.names = T, col.names = T, sep = ",")
write_tableHTML(
        tableHTML(data.summary[,c("state", "N", "time", "sd")]),
        file = outSummary)


# calculate details if available

#data.detail <- data.raw %>% filter(grepl("[0-9]\\.[0-9].*", state))
#outDetail <- paste(fileName, "-detail.csv", sep="")
#outDetail <- paste(dirname, outDetail, sep="/")

#if (nrow(data.detail) > 0) {
#    # calculate duration
#    data.detail$duration <- (data.detail$end - data.detail$start)
#    # grep iteration and push it to a separate column
#    data.detail <- mutate(data.detail, iter=gsub("([0-9]\\.[0-9].*Iteration_)([0-9]+)(_.*)", "\\2", data.detail$state))
#    # sort by duration
#    data.detail <- data.detail %>% arrange(desc(duration))
#    # better captions
#    data.detail$state <- gsub("(.*_[0-9]+_)(.*)",replacement = "\\2",data.detail$state)
#
#    # summarize if you want this
#    data.detail <- data.detail %>% group_by(unit,state) %>% summarize(median=median(duration), min=min(duration), max=max(duration))
#    data.detail <- data.detail %>% arrange(desc(max))
#    write.table(data.detail, outDetail, row.names = F, col.names = T, sep = ",")
#}
#
